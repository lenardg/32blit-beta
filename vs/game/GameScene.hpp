#pragma once

#include "SceneWithInputEvents.hpp"

#define RUN_VELOCITY 0.15f
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

struct gameState {
	vec2 playerpos;
	vec2 playervelocity;
	uint8_t current_floor;
	uint8_t current_level;
	uint32_t score;
	bool can_jump;
	gamefloor floors[FLOOR_COUNT];
} ;

class GameScene : public SceneWithInputEvents
{
private:
	gameState state;

public:
	virtual void init();
	virtual void render(uint32_t time);
	virtual void update(uint32_t time);

private:
	void add_random_hole(int level);
	void add_random_hole();
	void init_level();

	static bool compareHoles(hole h1, hole h2);
	void renderFloor(int floornumber, gamefloor floor);

	void checkHoles(bool& holeAbove, bool& holeBelow);
	void updatePosition();
	void updateHoles();

protected:
	virtual void on_Left();
	virtual void on_Right();
	virtual void on_A_pressed();
	virtual void on_B_pressed();
};

