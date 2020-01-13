#include "game.hpp"
#include <cmath>
#include <list>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace blit;

#define PLAYER_WIDTH 4
#define PLAYER_HEIGHT 10

#define SCREEN_WIDTH fb.bounds.w
#define SCREEN_HEIGHT fb.bounds.h

#define JUMP_VELOCITY -1.68f
#define GRAVITY 0.1f

#define CORRIDOR_HEIGHT 13
#define FLOOR_HEIGHT 1
#define FLOOR_COUNT 8
#define HOLE_WIDTH 12
#define HOLE_BASESPEED 0.25f

struct hole {
	float start;
	float speed;
};

struct gamefloor {
	std::vector<hole> holes;
};

struct {
	vec2 playerpos;
	vec2 playervelocity;
	uint8_t current_floor;
	uint8_t current_level;
	uint32_t score;
	bool can_jump;
	gamefloor floors[FLOOR_COUNT];
} state;

uint16_t last_buttons = 0;

///////////////////////////////////////////////////////////////////////////
//
// init_level()
//
// setup a new level
//
void add_random_hole(int level) {
	hole h;
	h.speed = (blit::random() % 2 == 0) ? HOLE_BASESPEED : -HOLE_BASESPEED;
	h.start = blit::random() % (SCREEN_WIDTH - HOLE_WIDTH);
	state.floors[level].holes.push_back(h);
}

void add_random_hole() {
	auto level = blit::random() % (FLOOR_COUNT - 1);
	add_random_hole(level);
}

void init_level() {
	state.playerpos.x = blit::random() % (SCREEN_WIDTH - PLAYER_WIDTH);
	state.playerpos.y = SCREEN_HEIGHT - PLAYER_HEIGHT;

	state.playervelocity.x = 0;
	state.playervelocity.y = 0;
	state.can_jump = true;
	state.current_floor = 0;

	for (int i = 0; i < FLOOR_COUNT; ++i) {
		state.floors[i].holes.clear();
		add_random_hole(i);
	}
}

///////////////////////////////////////////////////////////////////////////
//
// init()
//
// setup your game here
//
void init() {
	set_screen_mode(screen_mode::lores);

	state.current_level = 1;

	state.score = 0;

	init_level();
}

///////////////////////////////////////////////////////////////////////////
//
// render(time)
//
// This function is called to perform rendering of the game. time is the 
// amount if milliseconds elapsed since the start of your game
//
bool compareHoles(hole h1, hole h2) {
	return h1.start < h2.start;
}


void renderFloor(int floornumber, gamefloor floor) {
	std::vector<hole> holes;

	// add holes to our structure and sort
	for (auto ptr = floor.holes.begin(); ptr < floor.holes.end(); ++ptr) {
		holes.push_back(*ptr);
	}
	std::sort(holes.begin(), holes.end(), compareHoles);

	std::vector<int> points;
	points.push_back(0);
	for (auto ptr = holes.begin(); ptr < holes.end(); ++ptr) {
		points.push_back((*ptr).start);
		points.push_back((*ptr).start + HOLE_WIDTH);
	}
	points.push_back(SCREEN_WIDTH);

	fb.pen(blit::rgba(255, 255, 0));

	for (auto ptr = points.begin(); ptr < points.end(); ++ptr) {
		auto startx = *ptr;
		ptr++;
		auto endx = *ptr;
		fb.rectangle(rect(startx, SCREEN_HEIGHT - CORRIDOR_HEIGHT - floornumber * CORRIDOR_HEIGHT, endx - startx, FLOOR_HEIGHT));
	}
}

void render(uint32_t time) {

	// clear the screen -- fb is a reference to the frame buffer and can be used to draw all things with the 32blit
	fb.pen(blit::rgba(0, 0, 0));
	fb.clear();

	// draw some text at the top of the screen
	fb.alpha = 255;
	fb.mask = nullptr;
	fb.pen(rgba(255, 255, 255));
	fb.rectangle(rect(0, 0, 160, 14));
	fb.pen(rgba(0, 0, 0));

	fb.text("Level: " + std::to_string(state.current_level) + " - Score: " + std::to_string(state.score), &minimal_font[0][0], point(5, 4));

	for (int i = 0; i < 8; i++)
	{
		fb.pen(blit::rgba(10 + i * 7, 10 + i * 7, 10 + i * 7));
		fb.rectangle(rect(0, SCREEN_HEIGHT - CORRIDOR_HEIGHT - i * CORRIDOR_HEIGHT, SCREEN_WIDTH, CORRIDOR_HEIGHT));
		renderFloor(i, state.floors[i]);
	}

	fb.pen(blit::rgba(0, 200, 255));
	fb.rectangle(rect(state.playerpos.x, state.playerpos.y, PLAYER_WIDTH, PLAYER_HEIGHT));
}


void onLeft() {
	state.playervelocity.x -= 0.2f;
}

void onRight() {
	state.playervelocity.x += 0.2f;
}

void onJump() {
	if (state.can_jump) {
		state.can_jump = false;

		state.playervelocity.y = JUMP_VELOCITY;
	}
}

void checkKeys() {
	uint16_t changed = blit::buttons ^ last_buttons;
	uint16_t pressed = changed & blit::buttons;
	uint16_t released = changed & ~blit::buttons;

	if (blit::buttons & blit::button::DPAD_LEFT) {
		onLeft();
	}
	if (blit::buttons & blit::button::DPAD_RIGHT) {
		onRight();
	}

	if (pressed & button::A ) {
		onJump();
	}
}

void checkHoles(bool& holeAbove, bool& holeBelow) {
	holeAbove = false;
	if (state.current_floor < FLOOR_COUNT) {
		for (auto h : state.floors[state.current_floor].holes) {
			auto x1 = h.start;
			auto x2 = h.start + HOLE_WIDTH;

			if (state.playerpos.x >= x1 && state.playerpos.x + PLAYER_WIDTH <= x2) {
				holeAbove = true;
				break;
			}
		}
	}

	holeBelow = false;
	if (state.current_floor == 0) {
		return;
	}

	for (auto h : state.floors[state.current_floor - 1].holes) {
		auto x1 = h.start;
		auto x2 = h.start + HOLE_WIDTH;

		if (state.playerpos.x >= x1 && state.playerpos.x + PLAYER_WIDTH <= x2) {
			holeBelow = true;
			break;
		}
	}
}

void updatePosition() {
	state.playerpos.x += state.playervelocity.x;
	if (state.playerpos.x <= -PLAYER_WIDTH) {
		state.playerpos.x = SCREEN_WIDTH - 1;
	}
	else if (state.playerpos.x >= SCREEN_WIDTH + PLAYER_WIDTH) {
		state.playerpos.x = -PLAYER_WIDTH + 1;
	}

	state.playerpos.y += state.playervelocity.y;

	// check if player is standing under a hole
	bool holeAbove = false, holeBelow = false;
	checkHoles(holeAbove, holeBelow);

	// check if we hit the head
	if (!holeAbove && state.playerpos.y <= SCREEN_HEIGHT - (state.current_floor + 1) * CORRIDOR_HEIGHT) {
		state.playerpos.y = SCREEN_HEIGHT - (state.current_floor + 1) * CORRIDOR_HEIGHT + FLOOR_HEIGHT;
		state.playervelocity.y = 0;
	}

	// check if we reach next level
	if (holeAbove && state.playerpos.y + PLAYER_HEIGHT <= SCREEN_HEIGHT - (state.current_floor + 1) * CORRIDOR_HEIGHT) {
		state.current_floor++;
		add_random_hole();
		state.score += 10;
	}

	// check if we are falling down
	if (holeBelow && state.playerpos.y + PLAYER_HEIGHT > SCREEN_HEIGHT - (state.current_floor) * CORRIDOR_HEIGHT) {
		state.current_floor--;
		state.score -= 5;
	}

	// check if current level holds us
	if (!holeBelow && state.playerpos.y + PLAYER_HEIGHT > SCREEN_HEIGHT - (state.current_floor) * CORRIDOR_HEIGHT ) {
		state.playervelocity.y = 0;
		state.playerpos.y = SCREEN_HEIGHT - (state.current_floor) * CORRIDOR_HEIGHT - PLAYER_HEIGHT;
		state.can_jump = true;
	}
	// check if we are at the bottom of the screen
	else if (state.playerpos.y > SCREEN_HEIGHT - PLAYER_HEIGHT) {
		state.playervelocity.y = 0;
		state.playerpos.y = SCREEN_HEIGHT - PLAYER_HEIGHT;
		state.can_jump = true;
	}
	else {
		state.playervelocity.y += GRAVITY;
	}

	state.playervelocity.x *= 0.9f;

	if (abs(state.playervelocity.x) < 0.1f) {
		state.playervelocity.x = 0;
	}

	if (state.current_floor == FLOOR_COUNT) {
		state.current_level++;
		state.score += 100;
		init_level();
	}
}

void updateHoles() {
	for (int i = 0; i < FLOOR_COUNT; ++i) {
		for (auto ptr = state.floors[i].holes.begin(); ptr < state.floors[i].holes.end(); ++ptr) {
			hole& h = *ptr;
			h.start += h.speed;
			if (h.start < -HOLE_WIDTH) {
				h.start = SCREEN_WIDTH;
			}
			else if (h.start >= SCREEN_WIDTH) {
				h.start = -HOLE_WIDTH;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//
// update(time)
//
// This is called to update your game state. time is the 
// amount if milliseconds elapsed since the start of your game
//
void update(uint32_t time) {
	checkKeys();
	updatePosition();
	updateHoles();
}