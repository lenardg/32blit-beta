#include "firmware.hpp"
#include "graphics/color.hpp"
#include <cmath>
#include "quadspi.h"
#include "CDCCommandStream.h"
#include "USBManager.h"
#include "usbd_cdc_if.h"
#include "file.hpp"
#include "executable.hpp"
#include "metadata.hpp"
#include "dialog.hpp"
#include "engine/api_private.hpp"

#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <list>

using namespace blit;

enum State {stFlashFile, stSaveFile, stFlashCDC, stMassStorage};

constexpr uint32_t qspi_flash_sector_size = 64 * 1024;
constexpr uint32_t qspi_flash_size = 32768 * 1024;
constexpr uint32_t qspi_flash_address = 0x90000000;

extern CDCCommandStream g_commandStream;

FlashLoader flashLoader;
CDCEraseHandler cdc_erase_handler;

extern USBManager g_usbManager;

struct GameInfo {
  char title[25], author[17];
  char category[17];
  uint32_t size = 0, checksum = 0;

  uint32_t offset = ~0;
};

struct HandlerInfo {
  uint32_t offset, meta_offset;
  char type[5];
};

std::list<GameInfo> game_list;

std::list<HandlerInfo> handlers; // flashed games that can "launch" files

std::list<std::tuple<uint16_t, uint16_t>> free_space; // block start, count

uint32_t launcher_offset = ~0;

uint8_t buffer[PAGE_SIZE];
uint8_t verify_buffer[PAGE_SIZE];

State		state = stFlashFile;

FIL file;

Dialog dialog;

void scan_flash();
uint32_t flash_from_sd_to_qspi_flash(FIL &file, uint32_t flash_offset);

// metadata

bool parse_flash_metadata(uint32_t offset, GameInfo &info) {
  auto meta_offset = offset + info.size;
  auto game_offset = offset;

  uint8_t buf[10];
  if(qspi_read_buffer(meta_offset, buf, 10) != QSPI_OK)
    return false;

  if(memcmp(buf, "BLITMETA", 8) != 0)
    return false;

  RawMetadata raw_meta;
  if(qspi_read_buffer(meta_offset + 10, reinterpret_cast<uint8_t *>(&raw_meta), sizeof(RawMetadata)) != QSPI_OK) {
    return false;
  }

  info.size += *reinterpret_cast<uint16_t *>(buf + 8) + 10;
  info.checksum = raw_meta.crc32;
  memcpy(info.title, raw_meta.title, sizeof(info.title));
  memcpy(info.author, raw_meta.author, sizeof(info.author));

  offset = meta_offset + sizeof(RawMetadata) + 10;

  if(qspi_read_buffer(offset, buf, 8) != QSPI_OK)
    return true;

  if(memcmp(buf, "BLITTYPE", 8) != 0)
    return true;

  // type chunk
  RawTypeMetadata type_meta;
  if(qspi_read_buffer(offset + 8, reinterpret_cast<uint8_t *>(&type_meta), sizeof(RawTypeMetadata)) != QSPI_OK)
    return false;

  memcpy(info.category, type_meta.category, sizeof(info.category));

  offset += 8 + sizeof(RawTypeMetadata);

  // register handler
  HandlerInfo handler;
  handler.offset = game_offset;
  handler.meta_offset = meta_offset;
  handler.type[4] = 0;

  for(int i = 0; i < type_meta.num_filetypes; i++) {
    qspi_read_buffer(offset, (uint8_t *)handler.type, 5);
    offset += 5;
    handlers.push_back(handler);
  }

  return true;
}

bool parse_file_metadata(FIL &fh, GameInfo &info) {
  BlitGameHeader header;
  UINT bytes_read;
  bool result = false;
  f_lseek(&fh, 0);
  f_read(&fh, &header, sizeof(header), &bytes_read);

  // skip relocation data
  int off = 0;
  if(header.magic == 0x4F4C4552 /* RELO */) {
    f_lseek(&fh, 4);
    uint32_t num_relocs;
    f_read(&fh, (void *)&num_relocs, 4, &bytes_read);

    off = num_relocs * 4 + 8;
    f_lseek(&fh, off);

    // re-read header
    f_read(&fh, &header, sizeof(header), &bytes_read);
  }

  if(header.magic == blit_game_magic) {
    uint8_t buf[10];
    f_lseek(&fh, (header.end - 0x90000000) + off);
    auto res = f_read(&fh, buf, 10, &bytes_read);

    if(bytes_read == 10 && memcmp(buf, "BLITMETA", 8) == 0) {
      // don't bother reading the whole thing since we don't want the images
      const auto metadata_len = sizeof(RawMetadata);

      RawMetadata raw_meta;
      f_read(&fh, &raw_meta, sizeof(RawMetadata), &bytes_read);

      info.size += *reinterpret_cast<uint16_t *>(buf + 8) + 10;
      info.checksum = raw_meta.crc32;
      memcpy(info.title, raw_meta.title, sizeof(info.title));
      memcpy(info.author, raw_meta.author, sizeof(info.author));

      result = true;
    }

    // read category
    res = f_read(&fh, buf, 8, &bytes_read);
    if(bytes_read == 8 && memcmp(buf, "BLITTYPE", 8) == 0) {
      RawTypeMetadata type_meta;
      f_read(&fh, &type_meta, sizeof(RawTypeMetadata), &bytes_read);
      memcpy(info.category, type_meta.category, sizeof(info.category));
    }

  }

  return result;
}

int calc_num_blocks(uint32_t size) {
  return (size - 1) / qspi_flash_sector_size + 1;
}

void erase_qspi_flash(uint32_t start_sector, uint32_t size_bytes) {
  uint32_t sector_count = calc_num_blocks(size_bytes);

  progress.show("Erasing flash sectors...", sector_count);

  for(uint32_t sector = 0; sector < sector_count; sector++) {
    qspi_sector_erase((start_sector + sector) * qspi_flash_sector_size);

    progress.update(sector);
  }

  progress.hide();
}

void erase_flash_game(uint32_t offset) {
  // reject unaligned
  if(offset & (qspi_flash_sector_size - 1))
    return;

  // attempt to get size, falling back to a single sector
  int erase_size = 1;
  for(auto &game : game_list) {
    if(game.offset == offset) {
      erase_size = calc_num_blocks(game.size);
      break;
    }
  }

  bool flash_mapped = is_qspi_memorymapped();
  if(flash_mapped) {
    blit_disable_user_code();
    qspi_disable_memorymapped_mode();
  }

  erase_qspi_flash(offset / qspi_flash_sector_size, erase_size * qspi_flash_sector_size);

  // rescan
  scan_flash();

  if(flash_mapped) {
    qspi_enable_memorymapped_mode();
    blit_enable_user_code();
  }
}

// returns true is there is a valid header here
bool read_flash_game_header(uint32_t offset, BlitGameHeader &header) {
  if(qspi_read_buffer(offset, reinterpret_cast<uint8_t *>(&header), sizeof(header)) != QSPI_OK)
    return false;

  if(header.magic != blit_game_magic)
    return false;

  // make sure end/size is sensible
  if(header.end <= qspi_flash_address)
    return false;

  return true;
}

void scan_flash() {
  game_list.clear();
  free_space.clear();
  handlers.clear();

  GameInfo game;
  uint32_t free_start = 0xFFFFFFFF;

  for(uint32_t offset = 0; offset < qspi_flash_size;) {
    BlitGameHeader header;

    if(!read_flash_game_header(offset, header)) {
      if(free_start == 0xFFFFFFFF)
        free_start = offset;

      offset += qspi_flash_sector_size;
      continue;
    }

    game.offset = offset;
    game.size = header.end - qspi_flash_address;

    // check for valid metadata
    if(parse_flash_metadata(offset, game)) {
      // find the launcher
      if(strcmp(game.category, "launcher") == 0)
        launcher_offset = offset;

      // remove old firmware updates
      if(strcmp(game.title, "Firmware Updater") == 0 && persist.reset_target == prtFirmware) {
        int size_blocks = calc_num_blocks(game.size);

        erase_qspi_flash(offset / qspi_flash_sector_size, size_blocks * qspi_flash_sector_size);
        offset += size_blocks * qspi_flash_sector_size;
        continue;
      }
    }

    game_list.push_back(game);

    // add free space to list
    if(free_start != 0xFFFFFFFF) {
      auto start_block = free_start / qspi_flash_sector_size;
      auto end_block = offset / qspi_flash_sector_size;

      free_space.emplace_back(start_block, end_block - start_block);

      free_start = 0xFFFFFFFF;
    }

    offset += calc_num_blocks(game.size) * qspi_flash_sector_size;
  }

  // final free
  if(free_start != 0xFFFFFFFF) {
    auto start_block = free_start / qspi_flash_sector_size;
    auto end_block = qspi_flash_size / qspi_flash_sector_size;

    free_space.emplace_back(start_block, end_block - start_block);
  }
}

void launch_game(uint32_t address) {
  blit_switch_execution(address, false);
}

bool launch_game_from_sd(const char *path) {

  persist.launch_path[0] = 0;

  if(strncmp(path, "flash:/", 7) == 0) {
    blit_switch_execution(atoi(path + 7) * qspi_flash_sector_size, true);
    return true;
  }

  if(is_qspi_memorymapped()) {
    qspi_disable_memorymapped_mode();
    blit_disable_user_code(); // assume user running
  }

  uint32_t launch_offset = 0xFFFFFFFF;
  uint32_t flash_offset = launch_offset;

  // get the extension (assume there is one)
  std::string_view sv(path);
  auto ext = std::string(sv.substr(sv.find_last_of('.') + 1));
  for(auto &c : ext)
    c = tolower(c);

  if(ext != "blit") {
    // find the handler
    for(auto &handler : handlers) {
      if(handler.type == ext) {
        launch_offset = handler.offset;
        break;
      }
    }

    if(launch_offset == 0xFFFFFFFF)
      return false;

    // set the path to the file to launch
    strncpy(persist.launch_path, path, sizeof(persist.launch_path));

    blit_switch_execution(launch_offset, true);
    return true;

  }

  // .blit file, install/launch

  FIL file;
  FRESULT res = f_open(&file, path, FA_READ);
  if(res != FR_OK)
    return false;

  // get size
  // this is a little duplicated...
  FSIZE_t bytes_total = f_size(&file);
  char buf[8];
  UINT read;
  f_read(&file, buf, 8, &read);
  if(memcmp(buf, "RELO", 4) == 0) {
    auto num_relocs = *(uint32_t *)(buf + 4);
    bytes_total -= num_relocs * 4 + 8;
  }

  GameInfo meta;
  if(parse_file_metadata(file, meta)) {

    for(auto &flash_game : game_list) {
      // if a game with the same name/crc is already installed, launch that one instead of flashing it again
      if(flash_game.checksum == meta.checksum && strcmp(flash_game.title, meta.title) == 0) {
        launch_offset = flash_game.offset;
        break;
      } else if(strcmp(flash_game.title, meta.title) == 0 && strcmp(flash_game.author, meta.author) == 0) {
        // same game, different version
        if(calc_num_blocks(flash_game.size) <= calc_num_blocks(bytes_total)) {
          flash_offset = flash_game.offset;
          break;
        } else {
          // new version is bigger, erase old one
          erase_qspi_flash(flash_game.offset / qspi_flash_sector_size, flash_game.size);
        }
      } else if(strcmp(flash_game.category, "launcher") == 0 && strcmp(meta.category, "launcher") == 0) {
        // flashing a launcher, remove previous launchers
        erase_qspi_flash(flash_game.offset / qspi_flash_sector_size, flash_game.size);
      }
    }
  }

  if(launch_offset == 0xFFFFFFFF)
    launch_offset = flash_from_sd_to_qspi_flash(file, flash_offset);

  f_close(&file);

  if(launch_offset != 0xFFFFFFFF) {
    blit_switch_execution(launch_offset, true);
    return true;
  }

  blit_enable_user_code();

  return false;
}

static void *get_type_handler_metadata(const char *filetype) {
  for(auto &handler : handlers) {
    if(strncmp(filetype, handler.type, 4) == 0)
      return (void *)(qspi_flash_address + handler.meta_offset);
  }

  return nullptr;
}

static void start_launcher() {
  if(launcher_offset != 0xFFFFFFFF)
    launch_game(launcher_offset);
}

// used for updates
static bool launch_and_delete(const char *path) {
  FIL file;
  f_open(&file, path, FA_READ);

  GameInfo info;
  if(!parse_file_metadata(file, info))
    return false;

  auto offset = flash_from_sd_to_qspi_flash(file, 0xFFFFFFFF);

  f_close(&file);
  ::remove_file(path);

  launch_game(offset);

  return true;
}

void init() {
  api.launch = launch_game_from_sd;
  api.erase_game = erase_flash_game;
  api.get_type_handler_metadata = get_type_handler_metadata;

  set_screen_mode(ScreenMode::hires);
  screen.clear();

  scan_flash();

  // register PROG
  g_commandStream.AddCommandHandler(CDCCommandHandler::CDCFourCCMake<'P', 'R', 'O', 'G'>::value, &flashLoader);

  // register SAVE
  g_commandStream.AddCommandHandler(CDCCommandHandler::CDCFourCCMake<'S', 'A', 'V', 'E'>::value, &flashLoader);

  // register LS
  g_commandStream.AddCommandHandler(CDCCommandHandler::CDCFourCCMake<'_', '_', 'L', 'S'>::value, &flashLoader);

  g_commandStream.AddCommandHandler(CDCCommandHandler::CDCFourCCMake<'E', 'R', 'S', 'E'>::value, &cdc_erase_handler);

  // check for updates
  if(::file_exists("firmware-update.blit")) {
    // TODO: -vx.x.x?
    if(launch_and_delete("firmware-update.blit"))
      return;
  }

  // then launcher updates
  if(::file_exists("launcher.blit")) {
    // erase old launcher(s)
    for(auto &flash_game : game_list) {
      if(strcmp(flash_game.category, "launcher") == 0)
        erase_qspi_flash(flash_game.offset / qspi_flash_sector_size, flash_game.size);
    }

    if(launch_and_delete("launcher.blit"))
      return;
  }

  // auto-launch
  if(persist.reset_target == prtGame)
    launch_game(persist.last_game_offset);
  // error reset handling
  else if(persist.reset_error) {
    dialog.show("Oops!", "Restart game?", [](bool yes){

      if(yes)
        launch_game(persist.last_game_offset);
      else if(launcher_offset != 0xFFFFFFFF)
        start_launcher();

      persist.reset_error = false;
    });
  } else
    start_launcher();
}

void render(uint32_t time) {

  if(launcher_offset == 0xFFFFFFFF) {
    screen.pen = Pen(0, 0, 0);
    screen.clear();

    screen.pen = Pen(255, 255, 255);
    screen.text(
      "Please flash a launcher!\n\nUse \"32blit flash launcher.blit\"\nor place launcher.blit on your SD card.",
      minimal_font, Point(screen.bounds.w / 2, screen.bounds.h / 2), true, TextAlign::center_center
    );
  }

  progress.draw();
  dialog.draw();
}

void update(uint32_t time) {
  if(dialog.update())
    return;
}

// returns address to flash file to
uint32_t get_flash_offset_for_file(uint32_t file_size) {

  int file_blocks = calc_num_blocks(file_size);

  for(auto space : free_space) {
    if(std::get<1>(space) >= file_blocks)
      return std::get<0>(space) * qspi_flash_sector_size;
  }

  // TODO: handle flash full
  return 0;
}

// Flash a file from the SDCard to external flash
uint32_t flash_from_sd_to_qspi_flash(FIL &file, uint32_t flash_offset) {
  FRESULT res;

  // get file length
  FSIZE_t bytes_total = f_size(&file);
  UINT bytes_read = 0;
  FSIZE_t bytes_flashed = 0;

  size_t offset = 0;

  // check for prepended relocation info
  char buf[4];
  f_lseek(&file, 0);
  f_read(&file, buf, 4, &bytes_read);
  std::vector<uint32_t> relocation_offsets;
  size_t cur_reloc = 0;
  bool has_relocs = false;

  if(memcmp(buf, "RELO", 4) == 0) {
    uint32_t num_relocs;
    f_read(&file, (void *)&num_relocs, 4, &bytes_read);
    relocation_offsets.reserve(num_relocs);

    for(auto i = 0u; i < num_relocs; i++) {
      uint32_t reloc_offset;
      f_read(&file, (void *)&reloc_offset, 4, &bytes_read);

      relocation_offsets.push_back(reloc_offset - 0x90000000);
    }

    bytes_total -= num_relocs * 4 + 8; // size of relocation data
    has_relocs = true;
  } else {
    f_lseek(&file, 0);
  }

  // check header
  auto off = f_tell(&file);
  BlitGameHeader header;
  if(f_read(&file, (void *)&header, sizeof(header), &bytes_read) != FR_OK)
    return false;

  f_lseek(&file, off);

  if(!has_relocs)
    flash_offset = 0;
  else if(flash_offset == 0xFFFFFFFF)
    flash_offset = get_flash_offset_for_file(bytes_total);

  // erase the sectors needed to write the image
  erase_qspi_flash(flash_offset / qspi_flash_sector_size, bytes_total);

  progress.show("Copying from SD card to flash...", bytes_total);

  const int buffer_size = 4096;
  uint8_t buffer[buffer_size];
  uint8_t verify_buffer[buffer_size];

  while(bytes_flashed < bytes_total) {
    // limited ram so a bit at a time
    res = f_read(&file, (void *)buffer, buffer_size, &bytes_read);

    if(res != FR_OK)
      break;

    // relocation patching
    if(cur_reloc < relocation_offsets.size()) {
      for(auto off = offset; off < offset + bytes_read; off += 4) {
        if(off == relocation_offsets[cur_reloc]) {
          *(uint32_t *)(buffer + off - offset) += flash_offset;
          cur_reloc++;
        }
      }
    }

    if(qspi_write_buffer(offset + flash_offset, buffer, bytes_read) != QSPI_OK)
      break;

    if(qspi_read_buffer(offset + flash_offset, verify_buffer, bytes_read) != QSPI_OK)
      break;

    // compare buffers
    bool verified = true;
    for(uint32_t uB = 0; verified && uB < bytes_read; uB++)
      verified = buffer[uB] == verify_buffer[uB];

    if(!verified)
      break;

    offset += bytes_read;
    bytes_flashed += bytes_read;

    progress.update(bytes_flashed);
  }

  progress.hide();

  return bytes_flashed == bytes_total ? flash_offset : 0xFFFFFFFF;
}


void cdc_flash_list() {
  bool mapped = is_qspi_memorymapped();

  if(mapped)
    qspi_disable_memorymapped_mode();

  // scan through flash and send offset, size and metadata
  for(uint32_t offset = 0; offset < qspi_flash_size;) {
    BlitGameHeader header;

    if(!read_flash_game_header(offset, header)) {
      offset += qspi_flash_sector_size;
      continue;
    }

    uint32_t size = header.end - qspi_flash_address;

    // metadata header
    uint8_t buf[10];
    if(qspi_read_buffer(offset + size, buf, 10) != QSPI_OK)
      break;

    while(CDC_Transmit_HS((uint8_t *)&offset, 4) == USBD_BUSY){}
    while(CDC_Transmit_HS((uint8_t *)&size, 4) == USBD_BUSY){}

    uint16_t metadata_len = 0;

    if(memcmp(buf, "BLITMETA", 8) == 0)
      metadata_len = *reinterpret_cast<uint16_t *>(buf + 8);

    while(CDC_Transmit_HS((uint8_t *)"BLITMETA", 8) == USBD_BUSY){}
    while(CDC_Transmit_HS((uint8_t *)&metadata_len, 2) == USBD_BUSY){}

    // send metadata
    uint32_t metadata_offset = offset + size + 10;
    while(metadata_len) {
      int chunk_size = std::min(256, (int)metadata_len);
      uint8_t metadata_buf[256];

      if(qspi_read_buffer(metadata_offset, metadata_buf, chunk_size) != QSPI_OK)
        break;

      while(CDC_Transmit_HS(metadata_buf, chunk_size) == USBD_BUSY){}

      metadata_offset += chunk_size;
      metadata_len -= chunk_size;
    }

    offset += calc_num_blocks(size) * qspi_flash_sector_size;
  }

  // end marker
  uint32_t end = 0xFFFFFFFF;
  while(CDC_Transmit_HS((uint8_t *)&end, 4) == USBD_BUSY){}

  if(mapped)
    qspi_enable_memorymapped_mode();
}

// erase command handler
CDCCommandHandler::StreamResult CDCEraseHandler::StreamData(CDCDataStream &dataStream) {
  uint32_t offset;
  if(!dataStream.Get(offset))
    return srNeedData;

  erase_flash_game(offset);

  return srFinish;
}

//////////////////////////////////////////////////////////////////////
// Streaming Code
//  The streaming code works with a simple state machine,
//  current state is in m_parseState, the states parse index is
//  in m_uParseState
//////////////////////////////////////////////////////////////////////

// StreamInit() Initialise state machine
bool FlashLoader::StreamInit(CDCFourCC uCommand)
{
  //debugf("streamInit()\n\r");

  bool bNeedStream = true;
  switch(uCommand)
  {
    case CDCCommandHandler::CDCFourCCMake<'P', 'R', 'O', 'G'>::value:
      state = stFlashCDC;
      m_parseState = stFilename;
      m_uParseIndex = 0;

      flash_mapped = is_qspi_memorymapped();
      if(flash_mapped) {
        blit_disable_user_code();
        qspi_disable_memorymapped_mode();
      }
    break;

    case CDCCommandHandler::CDCFourCCMake<'S', 'A', 'V', 'E'>::value:
      state = stSaveFile;
      m_parseState = stFilename;
      m_uParseIndex = 0;
      blit_disable_user_code();
    break;

    case CDCCommandHandler::CDCFourCCMake<'_', '_', 'L', 'S'>::value:
      bNeedStream = false;
      cdc_flash_list();
    break;

  }

  return bNeedStream;
}


// FlashData() Flash data to the QSPI flash
// Note: currently qspi_write_buffer only works for sizes of 256 max
bool FlashData(uint32_t start, uint32_t uOffset, uint8_t *pBuffer, uint32_t uLen)
{
  bool bResult = false;
  if(QSPI_OK == qspi_write_buffer(start + uOffset, pBuffer, uLen))
  {
    if(QSPI_OK == qspi_read_buffer(start + uOffset, verify_buffer, uLen))
    {
      // compare buffers
      bResult = true;

      for(uint32_t uB = 0; bResult && uB < uLen; uB++)
        bResult = pBuffer[uB] == verify_buffer[uB];
    }
  }

  progress.update(uOffset + uLen);
  return bResult;
}


// SaveData() Saves date to file on SDCard
bool SaveData(uint8_t *pBuffer, uint32_t uLen)
{
  UINT uWritten;
  FRESULT res = f_write(&file, pBuffer, uLen, &uWritten);

  progress.update(f_tell(&file));

  return !res && (uWritten == uLen);
}


// StreamData() Handle streamed data
// State machine has three states:
// stFilename : Parse filename
// stLength   : Parse length, this is sent as an ascii string
// stData     : The binary data (.bin file)
CDCCommandHandler::StreamResult FlashLoader::StreamData(CDCDataStream &dataStream)
{
  CDCCommandHandler::StreamResult result = srContinue;
  uint8_t byte;
  while(dataStream.GetStreamLength() && result == srContinue)
  {
    switch (m_parseState)
    {
      case stFilename:
        if(m_uParseIndex < MAX_FILENAME)
        {
          while(result == srContinue && m_parseState == stFilename && dataStream.Get(byte))
          {
            m_sFilename[m_uParseIndex++] = byte;
            if (byte == 0)
            {
              m_parseState = stLength;
              m_uParseIndex = 0;
            }
          }
        }
        else
        {
          debugf("Failed to read filename\n\r");
          result =srError;
        }
      break;


      case stLength:
        if(m_uParseIndex < MAX_FILELEN)
        {
          while(result == srContinue && m_parseState == stLength && dataStream.Get(byte))
          {
            m_sFilelen[m_uParseIndex++] = byte;
            if (byte == 0)
            {
              m_parseState = stData;
              m_uParseIndex = 0;
              char *pEndPtr;
              m_uFilelen = strtoul(m_sFilelen, &pEndPtr, 10);
              if(m_uFilelen)
              {
                // init file or flash
                switch(state)
                {
                  case stSaveFile:
                  {
                    FRESULT res = f_open(&file, m_sFilename, FA_CREATE_ALWAYS | FA_WRITE);
                    if(res)
                    {
                      debugf("Failed to create file (%s)\n\r", m_sFilename);
                      result = srError;
                    } else {
                      char buf[300];
                      snprintf(buf, 300, "Saving %s to SD card...", m_sFilename);
                      progress.show(buf, m_uFilelen);
                    }
                  }
                  break;

                  case stFlashCDC:
                    m_parseState = stRelocs;
                  break;

                  default:
                  break;
                }
              }
              else
              {
                debugf("Failed to parse filelen\n\r");
                result =srError;
              }
            }
          }
        }
        else
        {
          debugf("Failed to read filelen\n\r");
          result =srError;
        }
      break;

      case stRelocs: {
        uint32_t word;
        if(m_uParseIndex > 1 && m_uParseIndex == num_relocs + 2) {
          cur_reloc = 0;
          m_parseState = stData;
          m_uParseIndex = 0;
        } else {
          while(result == srContinue && dataStream.Get(word)) {
            if(m_uParseIndex == 0 && word != 0x4F4C4552 /*RELO*/) {
              debugf("Missing relocation header\n");
              result = srError;
            } else if(m_uParseIndex == 1) {
              num_relocs = word;
              m_uFilelen -= num_relocs * 4 + 8;
              relocation_offsets.reserve(num_relocs);
            } else if(m_uParseIndex)
              relocation_offsets.push_back(word - 0x90000000);

            m_uParseIndex++;

            // done
            if(m_uParseIndex == num_relocs + 2)
              break;
          }
        }
        break;
      }

      case stData:
          while((result == srContinue) && (m_parseState == stData) && (m_uParseIndex <= m_uFilelen) && dataStream.Get(byte))
          {
            uint32_t uByteOffset = m_uParseIndex % PAGE_SIZE;
            buffer[uByteOffset] = byte;

            // check buffer needs writing
            volatile uint32_t uWriteLen = 0;
            bool bEOS = false;
            if (m_uParseIndex == m_uFilelen-1)
            {
              uWriteLen = uByteOffset+1;
              bEOS = true;
            }
            else
              if(uByteOffset == PAGE_SIZE-1)
                uWriteLen = PAGE_SIZE;

            if(uWriteLen)
            {
              switch(state)
              {
                case stSaveFile:
                  // save data
                  if(!SaveData(buffer, uWriteLen))
                  {
                    debugf("Failed to save to SDCard\n\r");
                    result = srError;
                  }

                  // end of stream close up
                  if(bEOS)
                  {
                    f_close(&file);

                    state = stFlashFile;
                    if(result != srError)
                      result = srFinish;

                    progress.hide();
                    blit_enable_user_code();
                  }
                break;

                case stFlashCDC:
                {
                  uint32_t uPage = (m_uParseIndex / PAGE_SIZE);
                  // first page, check header
                  if(uPage == 0) {
                    flash_start_offset = get_flash_offset_for_file(m_uFilelen);

                    // erase
                    erase_qspi_flash(flash_start_offset / qspi_flash_sector_size, m_uFilelen);

                    char buf[300];
                    snprintf(buf, 300, "Saving %s to flash...", m_sFilename);
                    progress.show(buf, m_uFilelen);
                  }

                  // relocation patching
                  if(cur_reloc < relocation_offsets.size()) {
                    auto offset = uPage * PAGE_SIZE;

                    for(auto off = offset; off < offset + uWriteLen; off += 4) {
                      if(off == relocation_offsets[cur_reloc]) {
                        *(uint32_t *)(buffer + off - offset) += flash_start_offset;
                        cur_reloc++;
                      }
                    }
                  }

                  // save data
                  if(!FlashData(flash_start_offset, uPage*PAGE_SIZE, buffer, uWriteLen))
                  {
                    debugf("Failed to write to flash\n\r");
                    result = srError;
                  }

                  // end of stream close up
                  if(bEOS)
                  {
                    if(result != srError)
                    {
                      result = srFinish;

                      // clean up old version(s)
                      BlitGameHeader header;
                      read_flash_game_header(flash_start_offset, header);

                      GameInfo meta;
                      meta.size = header.end - qspi_flash_address;
                      if(parse_flash_metadata(flash_start_offset, meta)) {
                        for(auto &game : game_list) {
                          if(strcmp(game.title, meta.title) == 0 && strcmp(game.author, meta.author) == 0 && game.offset != flash_start_offset) {
                            erase_qspi_flash(game.offset / qspi_flash_sector_size, game.size);
                          }
                          else if(strcmp(game.category, "launcher") == 0 && strcmp(meta.category, "launcher") == 0) {
                            // flashing a launcher, remove previous launchers
                            erase_qspi_flash(game.offset / qspi_flash_sector_size, game.size);
                          }
                        }
                      }

                      blit_switch_execution(flash_start_offset, true);
                    }
                    else
                      state = stFlashFile;

                    progress.hide();
                  }
                }
                break;

                default:
                break;
              }
            }

            m_uParseIndex++;
            m_uBytesHandled = m_uParseIndex;
          }
      break;
    }
  }

  if(result == srError) {
    state = stFlashFile;
    progress.hide();

    if(flash_mapped) {
      qspi_enable_memorymapped_mode();
      flash_mapped = false;
    }

    blit_enable_user_code();
  }

  return result;
}

