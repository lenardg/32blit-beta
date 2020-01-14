#include "PauseScene.hpp"
#include "SceneManager.hpp"

using namespace blit;
using namespace blit::oo;

PauseScene::PauseScene() {
	_isTransparent = true;
}

void PauseScene::render(uint32_t time) {
	fb.pen(rgba(40, 40, 40, 220));
	fb.rectangle(rect(0, 0, fb.bounds.w, fb.bounds.h));
	fb.pen(rgba(255, 255, 255));
	fb.text("PAUSED", &minimal_font[0][0], point(10, 20));
	fb.text("press X to continue", &minimal_font[0][0], point(10, 40));
}

void PauseScene::on_X_pressed() {
	exitScene();
}

void PauseScene::on_Menu_pressed() {
	exitScene();
}

