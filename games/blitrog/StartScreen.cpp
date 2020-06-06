#include "StartScreen.hpp"
#include "GameScene.hpp"

using namespace blit;
using namespace blit::oo;

const Pen colors[]{
	Pen(255,0,0),
	Pen(255,255,0),
	Pen(0,255,0),
	Pen(0,255,255),
	Pen(0,0,255),
	Pen(255,0,255)
};

StartScreen::StartScreen() {
	colorIndex = 0;
}

void StartScreen::init() {
	tween_bounce.init(tween_sine, 15, 0, 1000, -1);
	tween_bounce.start();
}

void StartScreen::render(uint32_t time) {
	screen.pen = Pen(50, 20, 40);
	screen.rectangle(Rect(0, 0, screen.bounds.w, screen.bounds.h));
	screen.pen = Pen(255, 255, 255);
	screen.text("BLITROG", minimal_font, Point(65, 15 + tween_bounce.value));

	screen.pen = Pen(140, 140, 140);
	screen.text("DPAD OR JOY to move", minimal_font, Point(10, 60));
	screen.text("A to jump", minimal_font, Point(10, 70));

	screen.pen = colors[colorIndex];
	screen.text("press A to start", outline_font, Point(40, 100));

	colorIndex++;
	if (colorIndex >= sizeof(colors) / sizeof(Pen)) {
		colorIndex = 0;
	}
}

void StartScreen::on_A_pressed() {
	tween_bounce.stop();
	SceneManager::getInstance().pushScene(new GameScene());
}

