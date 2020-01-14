#include "StartScreen.hpp"
#include "GameScene.hpp"

using namespace blit;
using namespace blit::oo;

const rgba colors[]{
	rgba(255,0,0),
	rgba(255,255,0),
	rgba(0,255,0),
	rgba(0,255,255),
	rgba(0,0,255),
	rgba(255,0,255)
};

StartScreen::StartScreen() {
	colorIndex = 0;
}

void StartScreen::render(uint32_t time) {
	fb.pen(rgba(50, 20, 40));
	fb.rectangle(rect(0, 0, fb.bounds.w, fb.bounds.h));
	fb.pen(rgba(255, 255, 255));
	fb.text("JUMPER", &minimal_font[0][0], point(65, 20));

	fb.pen(colors[colorIndex]);
	fb.text("press A to start", &outline_font[0][0], point(40, 100));

	colorIndex++;
	if (colorIndex >= sizeof(colors) / sizeof(rgba)) {
		colorIndex = 0;
	}
}

void StartScreen::on_A_pressed() {
	SceneManager::getInstance().pushScene(new GameScene());
}
