#include "game.hpp"
#include "StartScreen.hpp"

using namespace blit;
using namespace blit::oo;

Scene* startup() {
	set_screen_mode(screen_mode::lores);
	return new StartScreen();
}


