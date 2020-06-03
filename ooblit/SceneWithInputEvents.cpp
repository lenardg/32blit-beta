#include "SceneWithInputEvents.hpp"

namespace blit {
	namespace oo {
		void SceneWithInputEvents::checkKeys(uint32_t time) {
			Scene::checkKeys(time);

			if (isHeld(blit::Button::DPAD_UP)) {
				on_Up();
			}
			if (isHeld(blit::Button::DPAD_DOWN)) {
				on_Down();
			}
			if (isHeld(blit::Button::DPAD_LEFT)) {
				on_Left();
			}
			if (isHeld(blit::Button::DPAD_RIGHT)) {
				on_Right();
			}

			if (isPressed(blit::Button::A)) {
				on_A_pressed();
			}
			if (isPressed(blit::Button::B)) {
				on_B_pressed();
			}
			if (isPressed(blit::Button::X)) {
				on_X_pressed();
			}
			if (isPressed(blit::Button::Y)) {
				on_Y_pressed();
			}
			if (isPressed(blit::Button::MENU)) {
				on_Home_pressed();
			}
			if (isPressed(blit::Button::HOME)) {
				on_Menu_pressed();
			}
			if (isPressed(blit::Button::JOYSTICK)) {
				on_Joystick_pressed();
			}

			if (isReleased(blit::Button::A)) {
				on_A_released();
			}
			if (isReleased(blit::Button::B)) {
				on_B_released();
			}
			if (isReleased(blit::Button::X)) {
				on_X_released();
			}
			if (isReleased(blit::Button::Y)) {
				on_Y_released();
			}
			if (isReleased(blit::Button::MENU)) {
				on_Home_released();
			}
			if (isReleased(blit::Button::HOME)) {
				on_Menu_released();
			}
			if (isReleased(blit::Button::JOYSTICK)) {
				on_Joystick_released();
			}
		}
	}
}