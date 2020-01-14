#include "Scene.hpp"

Scene::Scene() {
	_isTransparent = false;
	_isAutoDeleting = true;
	_queuedToEnd = false;
}

Scene::Scene(const Scene& orig) {
	_isTransparent = orig._isTransparent;
	_isAutoDeleting = orig._isAutoDeleting;
}

Scene::~Scene() {
}

void Scene::init() {}
void Scene::render(uint32_t time) {}
void Scene::update(uint32_t time) {}

void Scene::execInit() {
	init();
}

void Scene::execRender(uint32_t time) {
	render(time);
}

bool Scene::execUpdate(uint32_t time) {
	checkKeys(time);
	update(time);
	return true;
}

void Scene::checkKeys(uint32_t time) {
	uint16_t changed = blit::buttons ^ _last_buttons;
	_pressed = changed & blit::buttons;
	_released = changed & ~blit::buttons;
}

bool Scene::isHeld(blit::button button) {
	return (blit::buttons & button);
}

bool Scene::isPressed(blit::button button) {
	return (_pressed & button);
}

bool Scene::isReleased(blit::button button) {
	return (_released & button);
}
