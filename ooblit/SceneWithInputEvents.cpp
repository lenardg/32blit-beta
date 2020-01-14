#include "SceneWithInputEvents.hpp"

namespace blit {
	namespace oo {
		void SceneWithInputEvents::checkKeys(uint32_t time) {
			Scene::checkKeys(time);

			if (isHeld(blit::button::DPAD_UP)) {
				on_Up();
			}
			if (isHeld(blit::button::DPAD_DOWN)) {
				on_Down();
			}
			if (isHeld(blit::button::DPAD_LEFT)) {
				on_Left();
			}
			if (isHeld(blit::button::DPAD_RIGHT)) {
				on_Right();
			}

			if (isPressed(blit::button::A)) {
				on_A_pressed();
			}
			if (isPressed(blit::button::B)) {
				on_B_pressed();
			}
			if (isPressed(blit::button::X)) {
				on_X_pressed();
			}
			if (isPressed(blit::button::Y)) {
				on_Y_pressed();
			}
			if (isPressed(blit::button::MENU)) {
				on_Home_pressed();
			}
			if (isPressed(blit::button::HOME)) {
				on_Menu_pressed();
			}
			if (isPressed(blit::button::JOYSTICK)) {
				on_Joystick_pressed();
			}

			if (isReleased(blit::button::A)) {
				on_A_released();
			}
			if (isReleased(blit::button::B)) {
				on_B_released();
			}
			if (isReleased(blit::button::X)) {
				on_X_released();
			}
			if (isReleased(blit::button::Y)) {
				on_Y_released();
			}
			if (isReleased(blit::button::MENU)) {
				on_Home_released();
			}
			if (isReleased(blit::button::HOME)) {
				on_Menu_released();
			}
			if (isReleased(blit::button::JOYSTICK)) {
				on_Joystick_released();
			}
		}
	}
}