#include "PauseScene.hpp"
#include "game.hpp"

using namespace blit;
using namespace blit::oo;

PauseScene::PauseScene() {
	_isTransparent = true;
}

void PauseScene::render(uint32_t time) {
	screen.pen = Pen(40, 40, 40, 220);
	screen.rectangle(Rect(0, 0, screen.bounds.w, screen.bounds.h));
	screen.pen = Pen(255, 255, 255);
	screen.text("PAUSED", minimal_font, Point(10, 20));
	screen.text("press X to continue", minimal_font, Point(10, 40));
}

void PauseScene::on_X_pressed() {
	exitScene();
}

void PauseScene::on_Menu_pressed() {
	exitScene();
}

