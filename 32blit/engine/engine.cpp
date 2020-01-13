/*! \file engine.cpp
*/
#include "engine.hpp"
#include "timer.hpp"
#include "tweening.hpp"

#include "../graphics/font.hpp"

namespace blit {

  void (*init)()                                = nullptr;
  void (*update)(uint32_t time)                 = nullptr;
  void (*render)(uint32_t time)                 = nullptr;
  void (*set_screen_mode)(screen_mode new_mode) = nullptr;
  uint32_t (*now)()                             = nullptr;
  uint32_t (*random)()                          = nullptr;
  void (*debug)(std::string message) = nullptr;
  int  (*debugf)(const char * psFormatString, ...) 	= nullptr;

  surface null_surface(nullptr, pixel_format::RGB565, size(0, 0));
  surface &fb = null_surface;

  uint32_t update_rate_ms = 10;
  uint32_t pending_update_time = 0;

  uint32_t render_rate_ms = 25;
  uint32_t pending_render_time = 0;

  uint32_t last_tick_time = 0;

  bool show_fps = false;
  float fps = 0.0f;
  uint32_t frame_count = 0;
  uint32_t fps_measurement_time = 0;

  void calculate_fps(uint32_t time) {
      frame_count++;
      if (time - fps_measurement_time > 1000) {
          float durationMultiplier = ((time - fps_measurement_time) / 1000.0f) / 1.0f;
          fps = (float)frame_count * durationMultiplier;
          fps_measurement_time = time;
          frame_count = 0;
      }
  }

  void show_fps_value() {
      std::string fpsstring;
      fpsstring.resize(16, 0);
      fpsstring.resize(snprintf(&fpsstring[0], 16, "%.1f", blit::fps));

      auto oldpen = fb.pen();

      fb.alpha = 192;
      fb.pen(rgba(30, 30, 30));
      fb.rectangle(rect(fb.bounds.w - 21, 0, 21, 12));
      fb.alpha = 255;
      fb.pen(rgba(0, 255, 0));
      fb.text(fpsstring, &minimal_font[0][0], point(fb.bounds.w - 20, 2));

      fb.pen(oldpen);
  }

  bool tick(uint32_t time) {
    bool has_rendered = false;

    if (last_tick_time == 0) {
      last_tick_time = time;
    }

    // update timers
    update_timers(time);
    update_tweens(time);

    // catch up on updates if any pending
    pending_update_time += (time - last_tick_time);
    while (pending_update_time >= update_rate_ms) {
      update(time - pending_update_time); // create fake timestamp that would have been accurate for the update event
      pending_update_time -= update_rate_ms;
    }

    // render if new frame due
    pending_render_time += (time - last_tick_time);
    if (pending_render_time >= render_rate_ms) {
      render(time);
      if (show_fps) {
          calculate_fps(time);
          show_fps_value();
      }
      pending_render_time -= render_rate_ms;
      has_rendered = true;
    }

    last_tick_time = time;
    return has_rendered;
  }

}
