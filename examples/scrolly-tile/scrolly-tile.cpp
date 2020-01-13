#include "scrolly-tile.hpp"
#include "graphics/color.hpp"

#define SCREEN_W blit::fb.bounds.w
#define SCREEN_H blit::fb.bounds.h

#define TILE_W 10
#define TILE_H 10
#define TILE_SOLID 0b1 << 7
#define TILE_WATER 0b1 << 6
#define TILE_LINKED 0b1 << 5

#define WALL_LEFT 1
#define WALL_RIGHT 2
#define WALL_NONE 0

#define PLAYER_W 2
#define PLAYER_H 4

#define TILES_Y uint8_t((SCREEN_H / TILE_H) + 3)
#define TILES_X uint8_t(SCREEN_W / TILE_W)

#define PLAYER_TOP player_position.y
#define PLAYER_BOTTOM player_position.y + PLAYER_H
#define PLAYER_RIGHT player_position.x + PLAYER_W
#define PLAYER_LEFT player_position.x

#define PLAYER_ALIVE 1
#define PLAYER_DEAD 0

// Number of times a player can jump sequentially
// including mid-air jumps and the initial ground
// or wall jump
#define MAX_JUMP 3

// All the art is rainbow fun time, so we don't need
// much data about each tile.
// Rounded corners are also procedural depending upon
// tile proximity.
// The screen allows for 16x12 10x10 tiles, but we
// use an extra 3 vertically:
// +1 - because an offset row means we can see 13 rows total
// +2 - because tile features need an adjacent row to generate from
// IE: when our screen is shifted down 5px you can see 13
// rows and both the top and bottom visible row are next to the
// additional two invisible rows which govern how corners are rounded.
uint8_t tiles[16 * 15] = { 0 };

blit::timer state_update;
blit::point tile_offset(0, 0);

vec2 player_position(80.0f, SCREEN_H - PLAYER_H);
vec2 player_velocity(0.0f, 0.0f);
vec2 jump_velocity(0.0f, -2.0f);

float water_level = 0;

uint32_t progress = 0;
uint16_t row_mask = 0xffff;
uint16_t linked_passage_mask = 0;
uint8_t passage_width = 0;
uint8_t last_passage_width = 0;

uint8_t passages[] = {
    7,
    10,
    0,
    1,
    4
};

uint8_t passage_count = 5;

uint16_t last_buttons = 0;
uint32_t jump_pressed = 0;
uint8_t can_jump = 0;
bool can_climb = 0;
uint8_t climbing_wall = WALL_NONE;
uint16_t player_tile_y = 0;
uint8_t player_status = PLAYER_ALIVE;

typedef uint16_t (*tile_callback)(uint16_t tile, uint8_t x, uint8_t y, void *args);

void for_each_tile(tile_callback callback, void *args) {
    for (auto y = 0; y < TILES_Y; y++) {
        for(auto x = 0; x < TILES_X; x++) {
            uint16_t index = (y * TILES_X) + x;
            tiles[index] = callback(tiles[index], x, y, args);
        }    
    }
}

uint16_t get_tile_at(uint8_t x, uint8_t y) {
    if (x < 0) return TILE_SOLID;
    if (x > 15) return TILE_SOLID;
    if (y > TILES_Y) return 0;
    if(y < 0) return TILE_SOLID;
    uint16_t index = (y * TILES_X) + x;
    return tiles[index];
}

#define TILE_LEFT        1 << 7
#define TILE_RIGHT       1 << 6
#define TILE_BELOW       1 << 5
#define TILE_ABOVE       1 << 4
#define TILE_ABOVE_LEFT  1 << 3
#define TILE_ABOVE_RIGHT 1 << 2
#define TILE_BELOW_LEFT  1 << 1
#define TILE_BELOW_RIGHT 1 << 0

uint16_t render_tile(uint16_t tile, uint8_t x, uint8_t y, void *args) {
    // Rendering tiles is pretty simple and involves drawing rectangles
    // in the right places.
    // But a large amount of this function is given over to rounding
    // corners depending upon the content of neighbouring tiles.
    // This could probably be rewritten to use a lookup table?
    blit::point offset = *(blit::point *)args;

    auto tile_x = (x * TILE_W) + offset.x;
    auto tile_y = (y * TILE_H) + offset.y;

    uint8_t feature_map = 0;

    auto tile_top = tile_y;
    auto tile_bottom = tile_y + TILE_H;
    auto tile_left = tile_x;
    auto tile_right = tile_x + TILE_W;

    feature_map |= (get_tile_at(x - 1, y) & TILE_SOLID) ? TILE_LEFT : 0;
    feature_map |= (get_tile_at(x + 1, y) & TILE_SOLID) ? TILE_RIGHT : 0;
    feature_map |= (get_tile_at(x, y - 1) & TILE_SOLID) ? TILE_ABOVE : 0;
    feature_map |= (get_tile_at(x, y + 1) & TILE_SOLID) ? TILE_BELOW : 0;

    feature_map |= (get_tile_at(x - 1, y - 1) & TILE_SOLID) ? TILE_ABOVE_LEFT : 0;
    feature_map |= (get_tile_at(x + 1, y - 1) & TILE_SOLID) ? TILE_ABOVE_RIGHT : 0;
    feature_map |= (get_tile_at(x - 1, y + 1) & TILE_SOLID) ? TILE_BELOW_LEFT : 0;
    feature_map |= (get_tile_at(x + 1, y + 1) & TILE_SOLID) ? TILE_BELOW_RIGHT : 0;

    rgba color_base = blit::hsv_to_rgba(tile_y / 120.0f, 0.5f, 0.8f);
    rgba color_dark = rgba(int(color_base.r * 0.75f), int(color_base.g * 0.75f), int(color_base.b * 0.75f));
    rgba color_darker = rgba(int(color_base.r * 0.5f), int(color_base.g * 0.5f), int(color_base.b * 0.5f));

    if(tile & TILE_SOLID) {
        blit::fb.pen(color_base);
        blit::fb.rectangle(rect(tile_x, tile_y, TILE_W, TILE_H));

        if((PLAYER_RIGHT > tile_left) && (PLAYER_LEFT < tile_right)){
            if(PLAYER_TOP < tile_bottom && PLAYER_BOTTOM > tile_bottom){
                blit::fb.pen(rgba(255, 255, 255));
                blit::fb.rectangle(rect(tile_x, tile_y, TILE_W, TILE_H));
            }
            if((PLAYER_BOTTOM > tile_top) && (PLAYER_TOP < tile_top)){
                blit::fb.pen(rgba(255, 255, 255));
                blit::fb.rectangle(rect(tile_x, tile_y, TILE_W, TILE_H));
            }
        }
        if((PLAYER_BOTTOM > tile_top) && (PLAYER_TOP < tile_bottom)){
            if(PLAYER_LEFT < tile_right && PLAYER_RIGHT > tile_right){
                blit::fb.pen(rgba(255, 255, 255));
                blit::fb.rectangle(rect(tile_x, tile_y, TILE_W, TILE_H));
            }
            if((PLAYER_RIGHT > tile_left) && (PLAYER_LEFT < tile_left)) {
                blit::fb.pen(rgba(255, 255, 255));
                blit::fb.rectangle(rect(tile_x, tile_y, TILE_W, TILE_H));
            }
        }

        if ((feature_map & (TILE_ABOVE_LEFT | TILE_ABOVE | TILE_LEFT)) == 0) {
            blit::fb.pen(rgba(0, 0, 0));
            blit::fb.pixel(point(tile_x, tile_y));
            blit::fb.pen(color_darker);
            blit::fb.pixel(point(tile_x + 1, tile_y));
            blit::fb.pixel(point(tile_x, tile_y + 1));
        }
        if ((feature_map & (TILE_ABOVE_RIGHT | TILE_ABOVE | TILE_RIGHT)) == 0) {
            blit::fb.pen(rgba(0, 0, 0));
            blit::fb.pixel(point(tile_x + TILE_W - 1, tile_y));
            blit::fb.pen(color_darker);
            blit::fb.pixel(point(tile_x + TILE_W - 2, tile_y));
            blit::fb.pixel(point(tile_x + TILE_W - 1, tile_y + 1));
        }
        if ((feature_map & (TILE_BELOW_LEFT | TILE_BELOW | TILE_LEFT)) == 0) {
            blit::fb.pen(rgba(0, 0, 0));
            blit::fb.pixel(point(tile_x, tile_y + TILE_H - 1));
            blit::fb.pen(color_darker);
            blit::fb.pixel(point(tile_x + 1, tile_y + TILE_H - 1));
            blit::fb.pixel(point(tile_x, tile_y + TILE_H - 2));
        }
        if ((feature_map & (TILE_BELOW_RIGHT | TILE_BELOW | TILE_RIGHT)) == 0) {
            blit::fb.pen(rgba(0, 0, 0));
            blit::fb.pixel(point(tile_x + TILE_W - 1, tile_y + TILE_H - 1));
            blit::fb.pen(color_darker);
            blit::fb.pixel(point(tile_x + TILE_W - 2, tile_y + TILE_H - 1));
            blit::fb.pixel(point(tile_x + TILE_W - 1, tile_y + TILE_H - 2));
        }
    } else {
        /*
        // Only useful for debugging - lets us see when the generator
        // links orphan passages back to those still in use
        if(tile & TILE_LINKED){
            blit::fb.pen(rgba(100, 100, 100));
            blit::fb.rectangle(rect(tile_x, tile_y, TILE_W, TILE_H));
        }
        */
        /*
        // This might have been a good idea but since we're getting all
        // neighbouring tiles anyway why don't we check for solid walls
        // all around us and fill with water?
        if(tile & TILE_WATER) {
            
            blit::fb.pen(rgba(100, 100, 255, 128));
            blit::fb.rectangle(rect(tile_x, tile_y + 5, TILE_W, 5));
        }
        */
        if(feature_map & TILE_ABOVE) {
            if (feature_map & TILE_LEFT) {
                blit::fb.pen(color_base);
                blit::fb.pixel(point(tile_x, tile_y));
                blit::fb.pen(color_dark);
                blit::fb.pixel(point(tile_x + 1, tile_y));
                blit::fb.pixel(point(tile_x, tile_y + 1));
            }
            if (feature_map & TILE_RIGHT) {
                blit::fb.pen(color_base);
                blit::fb.pixel(point(tile_x + TILE_W - 1, tile_y));
                blit::fb.pen(color_dark);
                blit::fb.pixel(point(tile_x + TILE_W - 2, tile_y));
                blit::fb.pixel(point(tile_x + TILE_W - 1, tile_y + 1));
            }
        }
        if(feature_map & TILE_BELOW) {
            // If we have a tile directly to the left and right
            // of this one then it's a little pocket we can fill with water!
            if(feature_map & TILE_LEFT && feature_map & TILE_RIGHT) {
                blit::fb.pen(rgba(200, 200, 255, 128));
                blit::fb.rectangle(rect(tile_x, tile_y + (TILE_H / 2), TILE_W, TILE_H / 2));
            }
            if(feature_map & TILE_LEFT) {
                blit::fb.pen(color_base);
                blit::fb.pixel(point(tile_x, tile_y + TILE_H - 1));
                blit::fb.pen(color_dark);
                blit::fb.pixel(point(tile_x + 1, tile_y + TILE_H - 1));
                blit::fb.pixel(point(tile_x, tile_y + TILE_H - 2));
            }
            if(feature_map & TILE_RIGHT) {
                blit::fb.pen(color_base);
                blit::fb.pixel(point(tile_x + TILE_W - 1, tile_y + TILE_H - 1));
                blit::fb.pen(color_dark);
                blit::fb.pixel(point(tile_x + TILE_W - 2, tile_y + TILE_H - 1));
                blit::fb.pixel(point(tile_x + TILE_W - 1, tile_y + TILE_H - 2));
            }
        }
    }

    return tile;
}

uint8_t count_set_bits(uint16_t number) {
    uint8_t count = 0;
    for(auto x = 0; x < 16; x++){
        if(number & (1 << x)){
            count++;
        }
    }
    return count;
}

void generate_new_row_mask() {
    uint16_t new_row_mask = 0x0000;
    linked_passage_mask = 0;

    // Cut our consistent winding passage through the level
    // by tracking the x coord of our passage we can ensure
    // that it's always navigable without having to reject
    // procedurally generated segments
    for(auto p = 0; p < passage_count; p++){
        if(p > passage_width) {
            continue;
        }
        // Controls how far a passage can snake left/right
        uint8_t turning_size = blit::random() % 7;

        new_row_mask |= (0x8000 >> passages[p]);

        // At every new generation we choose to branch a passage
        // either left or right, or let it continue upwards.
        switch(blit::random() % 3){
            case 0: // Passage goes right
                while(turning_size--){
                    if(passages[p] < TILES_X - 1){
                        passages[p] += 1;
                    }
                    new_row_mask |= (0x8000 >> passages[p]);
                }
                break;
            case 1: // Passage goes left
                while(turning_size--){
                    if(passages[p] > 0){
                        passages[p] -= 1;
                    }
                    new_row_mask |= (0x8000 >> passages[p]);
                }
                break;
        }
    }

    // Whenever we have a narrowing of our passage we must check
    // for orphaned passages and link them back to the ones still
    // available, to avoid the player going up a tunnel that ends
    // abruptly :(
    // This routine picks a random passage from the ones remaining
    // and routes every orphaned passage to it.
    if(passage_width < last_passage_width) {
        uint8_t target_passage = 0; //blit::random() % (passage_width + 1);
        uint8_t target_p_x = passages[target_passage];

        for(auto i = passage_width; i < last_passage_width + 1; i++){
            new_row_mask |= (0x8000 >> passages[i]);
            linked_passage_mask  |= (0x8000 >> passages[i]);

            int8_t direction = (passages[i] < target_p_x) ? 1 : -1;
    
            while(passages[i] != target_p_x) {
                passages[i] += direction;
                new_row_mask |= (0x8000 >> passages[i]);
                linked_passage_mask |= (0x8000 >> passages[i]);
            }
        }
    }
    last_passage_width = passage_width;

    row_mask = ~new_row_mask;
}

void update_tiles() {
    for(auto row = TILES_Y - 2; row > -1; row--){
        for(auto x = 0; x < TILES_X; x++){
            uint16_t tgt = ((row + 1) * TILES_X) + x;
            uint16_t src = (row * TILES_X) + x;
            tiles[tgt] = tiles[src];
        }
    }

    generate_new_row_mask();

    for(auto x = 0; x < TILES_X; x++) {
        if(row_mask & (1 << x)) {
            tiles[x] = TILE_SOLID;
        }
        else {
            tiles[x] = 0;
            if(linked_passage_mask & (1 << x)) {
                tiles[x] |= TILE_LINKED;
            }
        }
    }
}

void update_state(blit::timer &timer) {
    if (!(player_position.y < 70)) {
        return;
    }
    if(water_level > 10){
        water_level -= 1;
    }
    player_position.y += 1;
    progress += 1;
    passage_width = floorf(((sin(progress / 100.0f) + 1.0f) / 2.0f) * passage_count);
    tile_offset.y += 1;

    if(tile_offset.y >= 0) {
        tile_offset.y = -10;
        update_tiles();
    }
}

void place_player() {
    for(auto y = 10; y > 0; y--){
        for(auto x = 0; x < TILES_X; x++){
            uint16_t here = get_tile_at(x, y);
            uint16_t below = get_tile_at(x, y + 1);
            if(below & TILE_SOLID && (here & TILE_SOLID) == 0) {
                player_position.x = (x * TILE_W) + 4;
                player_position.y = (y * TILE_H) + tile_offset.y;
                return;
            }
        }
    }
}

void new_game() {
    player_status = PLAYER_ALIVE;

    // Use update_tiles to create the initial game state
    // instead of having a separate loop that breaks in weird ways
    for(auto x = 0; x < TILES_Y; x++) {
        update_tiles();
    }

    progress = 0;
    water_level = 0;
    passage_width = floorf(((sin(progress / 100.0f) + 1.0f) / 2.0f) * passage_count);
    player_velocity.x = 0.0f;
    player_velocity.y = 0.0f;

    place_player();

    state_update.start();
}

void init(void) {
    std::srand(12312897);
    blit::set_screen_mode(blit::lores);
    blit::show_fps = true;
    state_update.init(update_state, 10, -1);
    new_game();
}


uint16_t collide_player_lr(uint16_t tile, uint8_t x, uint8_t y, void *args) {
    blit::point offset = *(blit::point *)args;

    auto tile_x = (x * TILE_W) + offset.x;
    auto tile_y = (y * TILE_H) + offset.y;

    auto tile_top = tile_y;
    auto tile_bottom = tile_y + TILE_H;
    auto tile_left = tile_x;
    auto tile_right = tile_x + TILE_W;

    if(tile & TILE_SOLID) {
        if((PLAYER_BOTTOM > tile_top) && (PLAYER_TOP < tile_bottom)){
            // Collide the left-hand side of the tile right of player
            if(PLAYER_LEFT <= tile_right && PLAYER_RIGHT > tile_right){
                player_position.x = tile_right;
                player_velocity.x = 0.0f;
                jump_velocity.x = 0.5f;
                if(climbing_wall != WALL_LEFT) {
                    can_jump++;
                }
                can_climb = true;
                climbing_wall = WALL_LEFT;
            }
            // Collide the right-hand side of the tile left of player
            if((PLAYER_RIGHT >= tile_left) && (PLAYER_LEFT < tile_left)) {
                player_position.x = tile_left - PLAYER_W;
                player_velocity.x = 0.0f;
                jump_velocity.x = -0.5f;
                if(climbing_wall != WALL_RIGHT) {
                    can_jump++;
                }
                can_climb = true;
                climbing_wall = WALL_RIGHT;
            }
        }
    }

    return tile;
}

uint16_t collide_player_ud(uint16_t tile, uint8_t x, uint8_t y, void *args) {
    blit::point offset = *(blit::point *)args;

    auto tile_x = (x * TILE_W) + offset.x;
    auto tile_y = (y * TILE_H) + offset.y;

    auto tile_top = tile_y;
    auto tile_bottom = tile_y + TILE_H;
    auto tile_left = tile_x;
    auto tile_right = tile_x + TILE_W;

    if(tile & TILE_SOLID) {
        if((PLAYER_RIGHT > tile_left) && (PLAYER_LEFT < tile_right)){
            if(PLAYER_TOP < tile_bottom && PLAYER_BOTTOM > tile_bottom){
                player_position.y = tile_bottom;
                player_velocity.y = 0;
            }
            if((PLAYER_BOTTOM > tile_top) && (PLAYER_TOP < tile_top)){
                player_position.y = tile_top - PLAYER_H;
                player_velocity.y = 0;
                can_jump = MAX_JUMP;
                climbing_wall = WALL_NONE;
            }
        }
    }

    return tile;
}


void update(uint32_t time_ms) {
    uint16_t changed = blit::buttons ^ last_buttons;
    uint16_t pressed = changed & blit::buttons;
    uint16_t released = changed & ~blit::buttons;
    vec2 movement(0, 0);

    if(player_status == PLAYER_DEAD && pressed && blit::button::HOME){
        new_game();
        return;
    }

    if(player_status == PLAYER_ALIVE){
        water_level += 0.05f;

        if(blit::buttons & blit::button::DPAD_LEFT) {
            player_velocity.x -= 0.1f;
            movement.x = -1;
        }
        if(blit::buttons & blit::button::DPAD_RIGHT) {
            player_velocity.x += 0.1f;
            movement.x = 1;
        }
        if(blit::buttons & blit::button::DPAD_UP) {
            if(can_climb) {
                player_velocity.y -= 0.5;
            }
            movement.y = -1;
        }
        if(blit::buttons & blit::button::DPAD_DOWN) {
            movement.y = 1;
        }

        if(can_climb) {
            player_velocity.y *= 0.5f;
        }


        if(can_jump){
            if(pressed & blit::button::A) {
                player_velocity = jump_velocity;

                can_jump--;
            }
        }
    }

    player_velocity.y += 0.098f;
    player_velocity.y *= 0.99f;
    player_velocity.x *= 0.90f;


    player_position.x += player_velocity.x;
    // Useful for debug since you can position the player directly
    //player_position.x += movement.x;
    
    if(player_status == PLAYER_ALIVE) {
        jump_velocity.x = 0.0f;
        can_climb = false;
        if(player_position.x <= 0){
            player_position.x = 0;
            can_climb = true;
            if(climbing_wall != WALL_LEFT) {
                can_jump++;
            }
            climbing_wall = WALL_LEFT;
            jump_velocity.x = 0.5f;
        }
        if(player_position.x + PLAYER_W >= SCREEN_W) {
            player_position.x = SCREEN_W - PLAYER_W;
            can_climb = true;
            if(climbing_wall != WALL_RIGHT) {
                can_jump++;
            }
            climbing_wall = WALL_RIGHT;
            jump_velocity.x = -0.5f;

        }
    }
    for_each_tile(collide_player_lr, (void *)&tile_offset);


    player_position.y += player_velocity.y;
    // Useful for debug since you can position the player directly
    //player_position.y += movement.y;

    if(player_position.y + PLAYER_H > SCREEN_H) {
        // player_position.y = SCREEN_H - PLAYER_H;
        // player_velocity.y = 0;
        // can_jump = MAX_JUMP;
        state_update.stop();
        player_status = PLAYER_DEAD;
    }
    if(player_position.y > SCREEN_H - water_level) {
        state_update.stop();
        player_status = PLAYER_DEAD;
    }
    for_each_tile(collide_player_ud, (void *)&tile_offset);

    last_buttons = blit::buttons;
}

void render(uint32_t time_ms) {
    blit::fb.pen(blit::rgba(0, 0, 0));
    blit::fb.clear();
    for_each_tile(render_tile, (void *)&tile_offset);

    blit::fb.pen(blit::rgba(255, 255, 255));
    blit::fb.rectangle(rect(player_position.x, player_position.y, PLAYER_W, PLAYER_H));

    fb.pen(rgba(255, 255, 255));
    std::string p = std::to_string(progress);
    p.append("cm");
    fb.text(p, &minimal_font[0][0], point(2, 2));

    p = std::to_string(passage_width + 1);
    p.append(" passages");
    fb.text(p, &minimal_font[0][0], point(2, 10));

    if(water_level > 0){
        blit::fb.pen(blit::rgba(100, 100, 255, 200));
        blit::fb.rectangle(rect(0, SCREEN_H - water_level, SCREEN_W, water_level + 1));

        for(auto x = 0; x < SCREEN_W; x++){
            uint16_t offset = x + uint16_t(sin(time_ms / 500.0f) * 5.0f);
            if((offset % 5) > 0){
                blit::fb.pixel(point(x, SCREEN_H - water_level - 1));
            }
            if(((offset + 2) % 5) == 0){
                blit::fb.pixel(point(x, SCREEN_H - water_level - 2));
            }
            if(((offset + 3) % 5) == 0){
                blit::fb.pixel(point(x, SCREEN_H - water_level - 2));
            }
        }
    }

    if(player_status == PLAYER_DEAD) {
        fb.pen(rgba(128, 0, 0, 100));
        fb.rectangle(rect(0, 0, SCREEN_W, SCREEN_H));
        fb.pen(rgba(255, 0, 0, 255));
        fb.text("YOU DIED!", &minimal_font[0][0], point(SCREEN_H / 2 - 4, SCREEN_W / 2 - 20));
    }
}