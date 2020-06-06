#pragma once

#include "ooblit.hpp"

struct gameState {
} ;

class GameScene : public blit::oo::SceneWithInputEvents
{
private:
	gameState state;

public:
	virtual void init();
	virtual void render(uint32_t time);
	virtual void update(uint32_t time);

private:

protected:
};

