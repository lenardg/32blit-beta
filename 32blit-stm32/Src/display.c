#include <stdint.h>

#include "spi-st7272a.h"

#include "display.h"

extern char __ltdc_start, __ltdc_end;
extern char __fb_start, __fb_end;


void LTDC_IRQHandler() {
  if(((LTDC->ISR & LTDC_ISR_LIF) != RESET) && ((LTDC->IER & LTDC_IER_LIE) != RESET))
  {
    // disable line interrupt
    LTDC->IER &= ~LTDC_IT_LI;

    // clear interrupt flag
    LTDC->ICR = LTDC_FLAG_LI;

    display::flip(blit::fb);
    display::needs_render = true; 
  }
}

namespace display {  

  // surface mapped directly to the ltdc memory buffer
  surface __ltdc((uint8_t *)&__ltdc_start, pixel_format::RGB565, size(320, 240));

  // lo and hi res screen back buffers
  surface __fb_hires((uint8_t *)&__fb_start, pixel_format::RGB565, size(320, 240));
  surface __fb_lores((uint8_t *)&__fb_start, pixel_format::RGBA, size(160, 120));

  screen_mode mode = screen_mode::lores;
  bool needs_render = false;

  void init() {
    // TODO: replace interrupt setup with non HAL method
    HAL_NVIC_SetPriority(LTDC_IRQn, 4, 4);
    HAL_NVIC_EnableIRQ(LTDC_IRQn);

    ltdc_init();
    screen_init();

    enable_vblank_interrupt();
  }
  
  void enable_vblank_interrupt() {
    // disable line interrupt
    LTDC->IER &= ~LTDC_IT_LI;

    // trigger interrupt when screen refresh reaches the 252nd scanline
    LTDC->LIPCR = 252;

    // enable line interrupt
    LTDC->IER |= LTDC_IT_LI;

    display::needs_render = false;
  }

  void set_screen_mode(screen_mode new_mode) {
    mode = new_mode;
    fb = mode == screen_mode::hires ? __fb_hires : __fb_lores;
  }

  void flip(const surface &source) {
    uint32_t *s = (uint32_t *)source.data;
    uint32_t *d = (uint32_t *)(&__ltdc_start);

    if(mode == screen_mode::lores) {
      // pixel double the framebuffer to the ltdc buffer
      for(uint8_t y = 0; y < 120; y++) {
        // pixel double the current row while converting from RGBA to RGB565
        for(uint8_t x = 0; x < 160; x++) {
          uint16_t c = (((*s) & 0xf8000000) >> 27) | (((*s) & 0x00fc0000) >> 13) | (((*s) & 0x0000f800));        
          *(d) = c | (c << 16); *(d + 160) = c | (c << 16);
          d++; s++;
        }
        d += 160; // skip the doubled row
      }
    }else{
      // copy the framebuffer data into the ltdc buffer, originally this
      // was done via memcpy but implementing it as a 32-bit copy loop
      // was much faster. additionall unrolling this loop gained us about 10%
      // extra performance
      uint32_t c = (320 * 240) >> 4;
      while(c--) {
          *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
          *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;      
      }
    }

    // since the ltdc hardware pulls frame data directly over the memory bus
    // without passing through the mcu's cache layer we must invalidate the
    // affected area to ensure that all data has been committed into ram
    SCB_CleanInvalidateDCache_by_Addr(d, 320 * 240 * 2);    
  }

  void screen_init() {
    ST7272A_RESET();
    st7272a_set_bgr();
  }

  // TODO: could use some better structure and constants to make
  // what's happening clearer
  void ltdc_init() { 
    // configure ltdc peripheral

    __HAL_RCC_LTDC_CLK_ENABLE();
  
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    // LTDC GPIO Configuration    
    // PA1     ------> LTDC_R2
    // PA3     ------> LTDC_B5
    // PA4     ------> LTDC_VSYNC
    // PA6     ------> LTDC_G2
    // PB0     ------> LTDC_R3
    // PB1     ------> LTDC_R6
    // PE11     ------> LTDC_G3
    // PE12     ------> LTDC_B4
    // PE13     ------> LTDC_DE
    // PE14     ------> LTDC_CLK
    // PE15     ------> LTDC_R7
    // PB10     ------> LTDC_G4
    // PB11     ------> LTDC_G5
    // PC6     ------> LTDC_HSYNC
    // PC7     ------> LTDC_G6
    // PA8     ------> LTDC_B3
    // PA9     ------> LTDC_R5
    // PA11     ------> LTDC_R4
    // PD3     ------> LTDC_G7
    // PD6     ------> LTDC_B2
    // PB8     ------> LTDC_B6
    // PB9     ------> LTDC_B7 
    
      GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_6 
                          |GPIO_PIN_9|USB_DFU_DM___LTDC_R4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14 
                          |GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF13_LTDC;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);


    LTDC->GCR &= ~LTDC_GCR_PCPOL;   // synch signal polarity setting
    LTDC->SSCR = (3 << 16) | 3;     // hsync and vsync
    LTDC->BPCR = (46 << 16) | 15;   // accumulated horizonal and vertical back porch
    LTDC->AWCR = (366 << 16) | 255; // accumulated active width and height
    LTDC->TWCR = (374 << 16) | 257; // accumulated total width and height
    LTDC->BCCR = 0x00000000;        // background colour

    LTDC->IER = LTDC_IT_TE | LTDC_IT_FU; // enable transfer and fifo error interrupts

    // configure layer    
    rect window(0, 0, 320, 240);

    uint32_t tmp;

    tmp = ((window.w + ((LTDC->BPCR & LTDC_BPCR_AHBP) >> 16U)) << 16U);
    LTDC_Layer1->WHPCR &= ~(LTDC_LxWHPCR_WHSTPOS | LTDC_LxWHPCR_WHSPPOS);
    LTDC_Layer1->WHPCR = ((window.x + ((LTDC->BPCR & LTDC_BPCR_AHBP) >> 16U) + 1U) | tmp);
    tmp = ((window.h + (LTDC->BPCR & LTDC_BPCR_AVBP)) << 16U);
    LTDC_Layer1->WVPCR &= ~(LTDC_LxWVPCR_WVSTPOS | LTDC_LxWVPCR_WVSPPOS);
    LTDC_Layer1->WVPCR  = ((window.y + (LTDC->BPCR & LTDC_BPCR_AVBP) + 1U) | tmp);  
    LTDC_Layer1->PFCR   = LTDC_PIXEL_FORMAT_RGB565;  
    LTDC_Layer1->DCCR   = 0xff000000;     // layer default color (back, 100% alpha)
    LTDC_Layer1->CFBAR  = (uint32_t)&__ltdc_start;  // frame buffer start address
    LTDC_Layer1->CFBLR  = 320 * 2 << LTDC_LxCFBLR_CFBP_Pos | 320 * 2 << LTDC_LxCFBLR_CFBLL_Pos;  // frame buffer line length and pitch
    LTDC_Layer1->CFBLNR = 240;            // line count
    LTDC_Layer1->CACR   = 255;            // alpha
    LTDC_Layer1->CR    |= LTDC_LxCR_LEN;  // enable layer

    LTDC->SRCR = LTDC_SRCR_IMR;     // reload shadow registers
    LTDC->GCR |= LTDC_GCR_LTDCEN;   // enable LTDC      
  }
}