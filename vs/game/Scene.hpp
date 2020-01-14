#pragma once

#include "32blit.hpp"

class Scene {
protected:
	bool _isTransparent;
private:
	bool _isAutoDeleting;
	bool _queuedToEnd;
	uint16_t _last_buttons = 0;
	uint16_t _pressed;
	uint16_t _released;

protected:
	virtual void init();
	virtual void render(uint32_t time);
	virtual void update(uint32_t time);

public:
	virtual void execInit();
	virtual void execRender(uint32_t time);
	virtual bool execUpdate(uint32_t time);

protected:
	virtual void checkKeys(uint32_t time);

public:
	Scene();
	Scene(const Scene& sc);
	virtual ~Scene();

	bool isTransparent() {
		return _isTransparent;
	}

	bool isAutoDeleting() {
		return _isAutoDeleting;
	}

	void setAutoDeleting(bool shouldAutoDelete) {
		_isAutoDeleting = shouldAutoDelete;
	}

	void exitScene() {
		_queuedToEnd = true;
	}

	bool shouldExitScene() {
		return _queuedToEnd;
	}

protected:
	bool isPressed(blit::button button);
	bool isReleased(blit::button button);
	bool isHeld(blit::button button);
};
