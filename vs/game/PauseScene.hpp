#pragma once

#include "ooblit.hpp"

class PauseScene : public blit::oo::SceneWithInputEvents
{
public:
	PauseScene();
	virtual void render(uint32_t time);

protected:
	virtual void on_X_pressed();
	virtual void on_Home_pressed();
};

