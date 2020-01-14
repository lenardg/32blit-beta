#pragma once

#include "Scene.hpp"

namespace blit {
	namespace oo {
		class SceneWithInputEvents : public Scene {
		protected:
			virtual void checkKeys(uint32_t time);

		protected:
			virtual void on_Left() {}
			virtual void on_Right() {}
			virtual void on_Up() {}
			virtual void on_Down() {}

		protected:
			virtual void on_A_pressed() {}
			virtual void on_B_pressed() {}
			virtual void on_X_pressed() {}
			virtual void on_Y_pressed() {}
			virtual void on_Menu_pressed() {}
			virtual void on_Home_pressed() {}
			virtual void on_Joystick_pressed() {}

		protected:
			virtual void on_A_released() {}
			virtual void on_B_released() {}
			virtual void on_X_released() {}
			virtual void on_Y_released() {}
			virtual void on_Menu_released() {}
			virtual void on_Home_released() {}
			virtual void on_Joystick_released() {}

		};
	}
}
