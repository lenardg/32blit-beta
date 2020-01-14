#pragma once

#include "SceneWithInputEvents.hpp"

class PauseScene : public SceneWithInputEvents
{
public:
	PauseScene();
	virtual void render(uint32_t time);

protected:
	virtual void on_X_pressed();
	virtual void on_Menu_pressed();
};

