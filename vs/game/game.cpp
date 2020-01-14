#include "game.hpp"
#include "GameScene.hpp"

using namespace blit;

Scene* startup() {
	set_screen_mode(screen_mode::lores);
	return new GameScene();
}


