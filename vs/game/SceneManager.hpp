#pragma once

#include "32blit.hpp"
#include "Scene.hpp"
#include <list>

class SceneManager
{
private:
	std::list<Scene*> scenes;

public:
	static SceneManager& getInstance() {
		static SceneManager instance;
		return instance;
	}

private:
	SceneManager();

public:
	~SceneManager();

public:
	SceneManager(SceneManager const&) = delete;
	void operator=(SceneManager const&) = delete;

public:
	void pushScene(Scene* scene);
	void popScene();

public:
	void render(uint32_t time);
	void update(uint32_t time);
};

