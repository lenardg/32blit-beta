#include "GameScene.hpp"
#include "PauseScene.hpp"
#include "game.hpp"

#include <cmath>
#include <list>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace blit;
using namespace blit::oo;

#define PLAYER_WIDTH 4
#define PLAYER_HEIGHT 10

#define SCREEN_WIDTH screen.bounds.w
#define SCREEN_HEIGHT screen.bounds.h

///////////////////////////////////////////////////////////////////////////
//
// init_level()
//
// setup a new level
//
void GameScene::add_random_hole(int floorIndex) {
	while (true) {
		hole h;
		h.speed = (next_random(0, 2) == 0) ? HOLE_BASESPEED : -HOLE_BASESPEED;
		h.start = next_random(0, SCREEN_WIDTH - HOLE_WIDTH);

		//bool conflict = false;
		//for (auto xh : state.floors[level].holes) {
		//	if (xh.speed + h.speed != 0) {
		//		if ( xh.start )
		//	}
		//}
		//if (conflict) {
		//	continue;
		//}

		state.floors[floorIndex].holes.push_back(h);
		break;
	}
}

void GameScene::add_random_hole_to_random_floor( int excludeFloor ) {
	auto max_holes_per_floor = state.current_level + 1;
	if (max_holes_per_floor > 3) max_holes_per_floor = 3;

	auto max_holes = true;
	for ( auto i = 0; i < FLOOR_COUNT - 1; ++i ) {
		auto f = state.floors[i];
		if (f.holes.size() < max_holes_per_floor) {
			max_holes = false;
			break;
		}
	}
	if (max_holes) return;

	while (true) {
		auto floor = next_random(0, FLOOR_COUNT - 1);
		if (floor == excludeFloor) { continue; }
		if (state.floors[floor].holes.size() >= max_holes_per_floor) { continue; }
		add_random_hole(floor);
		break;
	}
}

void GameScene::init_level() {
	state.playerpos.x = next_random(0, SCREEN_WIDTH - PLAYER_WIDTH);
	state.playerpos.y = SCREEN_HEIGHT - PLAYER_HEIGHT;

	state.playervelocity.x = 0;
	state.playervelocity.y = 0;
	state.playerstate = playerState::READY;
	state.playerstatecounter = 0;
	state.dizzycounter = 0;
	
	state.lives = 3;

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
void GameScene::init() {
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
bool GameScene::compareHoles(hole h1, hole h2) {
	return h1.start < h2.start;
}


void GameScene::renderFloor(int floornumber, gamefloor floor) {
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

	screen.pen = Pen(255, 255, 0);

	for (auto ptr = points.begin(); ptr < points.end(); ++ptr) {
		auto startx = *ptr;
		ptr++;
		auto endx = *ptr;
		screen.rectangle(Rect(startx, SCREEN_HEIGHT - CORRIDOR_HEIGHT - floornumber * CORRIDOR_HEIGHT, endx - startx, FLOOR_HEIGHT));
	}
}

void GameScene::renderPlayer() {
	screen.pen = Pen(0, 200, 255);

	if (state.playerstate == playerState::READY) {
		screen.rectangle(Rect(state.playerpos.x, state.playerpos.y, PLAYER_WIDTH, PLAYER_HEIGHT));
	}
	else if (state.playerstate == playerState::RUNNING) {
		if (state.playerstatecounter % 36 < 12) {
			screen.rectangle(Rect(state.playerpos.x, state.playerpos.y, PLAYER_WIDTH, PLAYER_HEIGHT / 2));
			screen.rectangle(Rect(state.playerpos.x - 1, state.playerpos.y + PLAYER_HEIGHT / 2, PLAYER_WIDTH / 2, PLAYER_HEIGHT / 2));
			screen.rectangle(Rect(state.playerpos.x + PLAYER_WIDTH / 2, state.playerpos.y + PLAYER_HEIGHT / 2, PLAYER_WIDTH / 2, PLAYER_HEIGHT / 2));
		}
		else if (state.playerstatecounter % 36 < 24) {
			screen.rectangle(Rect(state.playerpos.x, state.playerpos.y, PLAYER_WIDTH, PLAYER_HEIGHT / 2));
			screen.rectangle(Rect(state.playerpos.x - 1, state.playerpos.y + PLAYER_HEIGHT / 2, PLAYER_WIDTH / 2, PLAYER_HEIGHT / 2));
			screen.rectangle(Rect(state.playerpos.x + PLAYER_WIDTH / 2 + 1, state.playerpos.y + PLAYER_HEIGHT / 2, PLAYER_WIDTH / 2, PLAYER_HEIGHT / 2));
		}
		else {
			screen.rectangle(Rect(state.playerpos.x, state.playerpos.y, PLAYER_WIDTH, PLAYER_HEIGHT));
		}
		state.playerstatecounter++;
	}
	else if (state.playerstate == playerState::DIZZY) {
		screen.rectangle(Rect(state.playerpos.x, state.playerpos.y + PLAYER_HEIGHT / 2 - 1, PLAYER_WIDTH, PLAYER_HEIGHT / 2));
		screen.rectangle(Rect(state.playerpos.x - PLAYER_HEIGHT / 2 + PLAYER_WIDTH / 2, state.playerpos.y + PLAYER_HEIGHT - 1, PLAYER_HEIGHT, 1));

		screen.pen = Pen(255, 255, 255);
		screen.pixel(
			Point(
				state.playerpos.x - 1 + (state.playerstatecounter % (PLAYER_WIDTH + 2)), 
				(int)(state.playerpos.y + PLAYER_HEIGHT / 2 - 3)));
		state.playerstatecounter++;
	}

}

void GameScene::render(uint32_t time) {

	// clear the screen -- fb is a reference to the frame buffer and can be used to draw all things with the 32blit
	screen.pen = Pen(0, 0, 0);
	screen.clear();

	// draw some text at the top of the screen
	screen.alpha = 255;
	screen.mask = nullptr;
	screen.pen = Pen(255, 255, 255);
	screen.rectangle(Rect(0, 0, 160, 14));
	screen.pen = Pen(0, 0, 0);

	screen.text("Level: " + std::to_string(state.current_level) + " - Score: " + std::to_string(state.score), minimal_font, Point(5, 4));

	for (int i = 0; i < 8; i++)
	{
		screen.pen = Pen(10 + i * 7, 10 + i * 7, 10 + i * 7);
		screen.rectangle(Rect(0, SCREEN_HEIGHT - CORRIDOR_HEIGHT - i * CORRIDOR_HEIGHT, SCREEN_WIDTH, CORRIDOR_HEIGHT));
		renderFloor(i, state.floors[i]);
	}

	renderPlayer();
}


void GameScene::on_Left() {
	if (state.playerstate != playerState::DIZZY) {
		state.playervelocity.x -= RUN_VELOCITY;
		if (state.playervelocity.x < -MAX_RUN_VELOCITY) {
			state.playervelocity.x = -MAX_RUN_VELOCITY;
		}
	}	
}

void GameScene::on_Right() {
	if (state.playerstate != playerState::DIZZY) {
		state.playervelocity.x += RUN_VELOCITY;
		if (state.playervelocity.x > MAX_RUN_VELOCITY) {
			state.playervelocity.x = MAX_RUN_VELOCITY;
		}
	}
}

void GameScene::on_A_pressed() {
	if (state.can_jump && state.playerstate != playerState::DIZZY) {
		state.can_jump = false;

		state.playervelocity.y = JUMP_VELOCITY;
	}
}

void GameScene::on_Menu_pressed() {
	SceneManager::getInstance().pushScene(new PauseScene());
}

void GameScene::checkHoles(bool& holeAbove, bool& holeBelow) {
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

void GameScene::updatePosition() {
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

		state.playerstate = playerState::DIZZY;
		state.dizzycounter = DIZZY_TIME;
	}

	// check if we reach next level
	if (holeAbove && state.playerpos.y + PLAYER_HEIGHT <= SCREEN_HEIGHT - (state.current_floor + 1) * CORRIDOR_HEIGHT) {
		add_random_hole_to_random_floor(state.current_floor);
		state.current_floor++;
		state.score += 10;
	}

	// check if we are falling down
	if (holeBelow && state.playerpos.y + PLAYER_HEIGHT > SCREEN_HEIGHT - (state.current_floor) * CORRIDOR_HEIGHT) {
		state.current_floor--;
		state.score -= 5;

		state.playerstate = playerState::DIZZY;
		state.dizzycounter = DIZZY_TIME;
	}

	// check if current level holds us
	if (!holeBelow && state.playerpos.y + PLAYER_HEIGHT > SCREEN_HEIGHT - (state.current_floor) * CORRIDOR_HEIGHT) {
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
		if (state.playerstate != playerState::DIZZY) {
			state.playerstate = playerState::READY;
		}
	}
	else if ( state.playerstate != playerState::DIZZY ) {
		state.playerstate = playerState::RUNNING;
	}

	if (state.current_floor == FLOOR_COUNT) {
		state.current_level++;
		state.score += 100;
		init_level();
	}

	if (state.playerstate == playerState::DIZZY) {
		if (state.dizzycounter > 0) {
			state.dizzycounter--;
		}
		else {
			state.playerstate = playerState::READY;
		}
	}
}

void GameScene::updateHoles() {
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
void GameScene::update(uint32_t time) {
	updatePosition();
	updateHoles();
}
