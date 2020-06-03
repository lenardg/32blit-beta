#pragma once

#include "ooblit.hpp"

class StartScreen : public blit::oo::SceneWithInputEvents
{
public:
	StartScreen();
	virtual void init();
	virtual void render(uint32_t time);

protected:
	virtual void on_A_pressed();

private:
	int colorIndex;
	blit::tween tween_bounce;
};

