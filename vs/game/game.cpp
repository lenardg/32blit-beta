#include "game.hpp"
#include "GameScene.hpp"
#include "SceneManager.hpp"

using namespace blit;

///////////////////////////////////////////////////////////////////////////
//
// init()
//
// setup your game here
//
void init() {
	set_screen_mode(screen_mode::lores);

	SceneManager::getInstance().pushScene(new GameScene());
}

///////////////////////////////////////////////////////////////////////////
//
// render(time)
//
// This function is called to perform rendering of the game. time is the 
// amount if milliseconds elapsed since the start of your game
//
void render(uint32_t time) {
	SceneManager::getInstance().render(time);
}

///////////////////////////////////////////////////////////////////////////
//
// update(time)
//
// This is called to update your game state. time is the 
// amount if milliseconds elapsed since the start of your game
//
void update(uint32_t time) {
	SceneManager::getInstance().update(time);
}