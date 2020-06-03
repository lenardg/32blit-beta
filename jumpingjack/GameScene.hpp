#pragma once

#include "ooblit.hpp"

#define RUN_VELOCITY 0.15f
#define JUMP_VELOCITY -1.68f
#define GRAVITY 0.1f

#define CORRIDOR_HEIGHT 13
#define FLOOR_HEIGHT 1
#define FLOOR_COUNT 8
#define HOLE_WIDTH 12
#define HOLE_BASESPEED 0.25f

#define DIZZY_TIME 200

struct hole {
	float start;
	float speed;
};

struct gamefloor {
	std::vector<hole> holes;
};

enum class playerState {
	READY,
	RUNNING,
	DIZZY,
};

struct gameState {
	vec2 playerpos;
	vec2 playervelocity;
	playerState playerstate;
	uint16_t playerstatecounter;
	uint8_t lives;
	uint16_t dizzycounter;

	uint8_t current_floor;
	uint8_t current_level;
	uint32_t score;
	bool can_jump;
	gamefloor floors[FLOOR_COUNT];
} ;

class GameScene : public blit::oo::SceneWithInputEvents
{
private:
	gameState state;

public:
	virtual void init();
	virtual void render(uint32_t time);
	virtual void update(uint32_t time);

private:
	void add_random_hole(int floorIndex);
	void add_random_hole_to_random_floor( int excludeFloor = -1);
	void init_level();

	static bool compareHoles(hole h1, hole h2);
	void renderFloor(int floornumber, gamefloor floor);
	void renderPlayer();

	void checkHoles(bool& holeAbove, bool& holeBelow);
	void updatePosition();
	void updateHoles();

protected:
	virtual void on_Left();
	virtual void on_Right();
	virtual void on_A_pressed();
	virtual void on_Menu_pressed();
};

