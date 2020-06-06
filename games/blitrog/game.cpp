#include "game.hpp"
#include "GameScene.hpp"
#include "StartScreen.hpp"

using namespace blit;
using namespace blit::oo;

Scene* startup() {
	set_screen_mode(ScreenMode::lores);
	return new StartScreen();
}