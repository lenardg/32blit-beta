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

#define SCREEN_WIDTH screen.bounds.w
#define SCREEN_HEIGHT screen.bounds.h

///////////////////////////////////////////////////////////////////////////
//
// init()
//
// setup your game here
//
void GameScene::init() {
}

///////////////////////////////////////////////////////////////////////////
//
// render(time)
//
// This function is called to perform rendering of the game. time is the 
// amount if milliseconds elapsed since the start of your game
//
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

//	screen.text("Level: " + std::to_string(state.current_level) + " - Score: " + std::to_string(state.score), minimal_font, Point(5, 4));
	screen.text("BLITROG", minimal_font, Point(5, 4));
}


///////////////////////////////////////////////////////////////////////////
//
// update(time)
//
// This is called to update your game state. time is the 
// amount if milliseconds elapsed since the start of your game
//
void GameScene::update(uint32_t time) {
}
