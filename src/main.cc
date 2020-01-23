// This file is part of xturtle.
//
// xturtle is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// xturtle is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with xturtle.  If not, see <https://www.gnu.org/licenses/>.

#include <iostream>

#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>
#include <spdlog/spdlog.h>

#include "config.hh"

// The width and height of the window in pixels.
const uint16_t WIDTH = 500;
const uint16_t HEIGHT = 500;

/// Run the event loop.
void run(xcb_connection_t *connection, cairo_surface_t *surface) {
  auto *cr = cairo_create(surface);

  spdlog::debug("Starting event loop...");

  xcb_generic_event_t *event;
  while ((event = xcb_wait_for_event(connection))) {
    switch (event->response_type & ~0x80) {
    case XCB_EXPOSE:
      // Avoid extra redraws by checking if this is
      // the last expose event in the sequence.
      if (((xcb_expose_event_t *) event)->count != 0) {
        break;
      }

      // White background
      cairo_set_source_rgb(cr, 1, 1, 1);
      cairo_paint(cr);

      // Diagonal line
      cairo_move_to(cr, 5, 5);
      cairo_set_line_width(cr, 3);
      cairo_set_source_rgb(cr, 0, 0, 0);
      cairo_line_to(cr, 495, 495);
      cairo_stroke(cr);

      cairo_surface_flush(surface);
      break;
    }

    free(event);
    xcb_flush(connection);
  }
}

int main() {
#ifdef XTURTLE_DEBUG_ENABLED
  spdlog::set_level(spdlog::level::debug);
#endif

  // The screen that the server prefers. On modern interactive desktops, there
  // typically is only 1 screen shared amongst the displays with XRANDR or
  // similar, and thus this is commonly the single screen #0.
  int screen_number;

  auto *connection = xcb_connect(nullptr, &screen_number);
  if (xcb_connection_has_error(connection)) {
    spdlog::critical("Could not connect to the X server");
    return 1;
  } else {
    spdlog::info("Connected to X server");
  }

  auto screen = xcb_aux_get_screen(connection, screen_number);
  if (!screen) {
    spdlog::critical("Could not access screen #{}", screen_number);
    exit(1);
  } else {
    spdlog::debug("Displaying on screen #{}:", screen_number);
    spdlog::debug("  root window ID: 0x{0:x}", screen->root);
    spdlog::debug("  root visual ID: 0x{0:x}", screen->root_visual);
    spdlog::debug("  dimensions: {}x{}",
        screen->width_in_pixels,
        screen->height_in_pixels
    );
  }

  // FIXME: This does not account for the window size, which means that the
  //        *top left* of the window is centred, not the window itself.
  int16_t xpos = screen->width_in_pixels / 2;
  int16_t ypos = screen->height_in_pixels / 2;

  uint32_t mask[2];
  mask[0] = 1;
  mask[1] = XCB_EVENT_MASK_EXPOSURE;

  auto window = xcb_generate_id(connection);
  xcb_create_window(connection,
                    XCB_COPY_FROM_PARENT,           // Depth
                    window,                         // ID
                    screen->root,                   // Parent window
                    xpos, ypos,                     // Position (x, y)
                    WIDTH, HEIGHT,                  // Size
                    2,                              // Border width
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,  // Class
                    XCB_COPY_FROM_PARENT,           // Visual ID
                    XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK, mask  // Masks
  );
  xcb_map_window(connection, window);

  auto visual_type = xcb_aux_find_visual_by_id(screen, screen->root_visual);
  auto *surface = cairo_xcb_surface_create(
    connection, window, visual_type, WIDTH, HEIGHT
  );

  xcb_flush(connection);

  run(connection, surface);

  cairo_surface_finish(surface);
  xcb_disconnect(connection);

  return 0;
}
