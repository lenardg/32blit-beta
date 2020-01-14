#include "SceneManager.hpp"
#include <queue>

SceneManager::SceneManager() {

}

SceneManager::~SceneManager() {

}

void SceneManager::pushScene(Scene* scene) {
	scenes.push_back(scene);
	scene->execInit();
}

void SceneManager::popScene() {
	if (scenes.size() == 0) return;

	auto lastScene = scenes.back();
	scenes.pop_back();

	if (lastScene->isAutoDeleting()) {
		delete lastScene;
	}
}

void SceneManager::render(uint32_t time) {
	auto ptr = scenes.end();
	ptr--;
	while ((*ptr)->isTransparent()) {
		--ptr;
	}
	while (ptr != scenes.end()) {
		(*ptr)->execRender(time);
		ptr++;
	}
}

void SceneManager::update(uint32_t time) {
	std::queue<Scene*> processingQueue;

	auto ptr = scenes.end();
	while (ptr != scenes.begin()) {
		--ptr;
		processingQueue.push(*ptr);
		if (!(*ptr)->isTransparent()) { break; }
	}

	while (processingQueue.size()) {
		auto nextScene = processingQueue.front();
		auto result = nextScene->execUpdate(time);
		if (nextScene->shouldExitScene()) {
			popScene();
		}
		if (result) {
			break;
		}

		processingQueue.pop();
	}
}
