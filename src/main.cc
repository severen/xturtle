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
#include <cmath>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#include "config.hh"

// The width and height of the window in pixels.
const uint16_t WIDTH = 500;
const uint16_t HEIGHT = 500;

/// Draw a line from the point (x1, y1) to the point (x2, y2).
void draw_line(cairo_t *cr, double x1, double y1, double x2, double y2) {
  cairo_move_to(cr, x1, y1);
  cairo_line_to(cr, x2, y2);
  cairo_stroke(cr);
}

struct Pen {
  double red;
  double green;
  double blue;
  double thickness;
  bool down;

  Pen(): red(0), green(0), blue(0), thickness(0.5), down(true) {}
};

class Turtle {
  private:
    double x = 0;
    double y = 0;
    double direction = 0;
    Pen pen;

  public:
    void pen_up() {
      this->pen.down = false;
    }

    void pen_down() {
      this->pen.down = true;
    }

    void turn(double degrees) {
      this->direction += M_PI / 180.0 * degrees;
    }

    void move(cairo_t *cr, double distance) {
      double new_x = this->x + distance * cos(this->direction);
      double new_y = this->y + distance * sin(this->direction);

      if (this->pen.down) {
        draw_line(cr, this->x, this->y, new_x, new_y);
      }

      this->x = new_x;
      this->y = new_y;
    }

    void reset() {
      this->x = 0;
      this->x = 0;
      this->direction = 0;
      this->pen = Pen();
    }

    void set_pen_color(double red, double green, double blue) {
      this->pen.red = red;
      this->pen.green = green;
      this->pen.blue = blue;
    }

    void set_pen_thickness(double thickness) {
      this->pen.thickness = thickness;
    }
};

struct State {
  Turtle turtle;

  xcb_connection_t *connection;
  xcb_screen_t *screen;
  xcb_window_t window;
  cairo_surface_t *surface;

  State() {
    // The screen that the server prefers. On modern interactive desktops, there
    // typically is only 1 screen shared amongst the displays with XRANDR or
    // similar, and thus this is commonly the single screen #0.
    int screen_number;

    this->connection = xcb_connect(nullptr, &screen_number);
    if (xcb_connection_has_error(this->connection)) {
      spdlog::critical("Could not connect to the X server");
      exit(1);
    } else {
      spdlog::info("Connected to X server");
    }

    this->screen = xcb_aux_get_screen(this->connection, screen_number);
    if (!this->screen) {
      spdlog::critical("Could not access screen #{}", screen_number);
      exit(1);
    } else {
      spdlog::debug("Displaying on screen #{}:", screen_number);
      spdlog::debug("  root window ID: 0x{0:x}", this->screen->root);
      spdlog::debug("  root visual ID: 0x{0:x}", this->screen->root_visual);
      spdlog::debug("  dimensions: {}x{}",
          this->screen->width_in_pixels,
          this->screen->height_in_pixels
      );
    }

    // FIXME: This does not account for the window size, which means that the
    //        *top left* of the window is centred, not the window itself.
    int16_t xpos = this->screen->width_in_pixels / 2;
    int16_t ypos = this->screen->height_in_pixels / 2;

    uint32_t mask[2];
    mask[0] = 1;
    mask[1] = XCB_EVENT_MASK_EXPOSURE;

    this->window = xcb_generate_id(this->connection);
    xcb_create_window(this->connection,
                      XCB_COPY_FROM_PARENT,           // Depth
                      this->window,                   // ID
                      this->screen->root,             // Parent window
                      xpos, ypos,                     // Position (x, y)
                      WIDTH, HEIGHT,                  // Size
                      2,                              // Border width
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,  // Class
                      XCB_COPY_FROM_PARENT,           // Visual ID
                      XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK, mask  // Masks
    );
    xcb_map_window(this->connection, this->window);

    auto visual_type = xcb_aux_find_visual_by_id(this->screen, this->screen->root_visual);
    this->surface = cairo_xcb_surface_create(
      this->connection, this->window, visual_type, WIDTH, HEIGHT
    );
  }

  ~State() {
    cairo_surface_finish(this->surface);
    xcb_disconnect(this->connection);
  }
};

int main(int argc, char *argv[]) {
  CLI::App app;
  CLI11_PARSE(app, argc, argv);

#ifdef XTURTLE_DEBUG_ENABLED
  spdlog::set_level(spdlog::level::debug);
#endif

  State state;

  // Send all queued commands to the server.
  xcb_flush(state.connection);

  spdlog::debug("Starting event loop...");
  auto *cr = cairo_create(state.surface);

  xcb_generic_event_t *event;
  while ((event = xcb_wait_for_event(state.connection))) {
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
      cairo_set_line_width(cr, 3);
      cairo_set_source_rgb(cr, 0, 0, 0);
      state.turtle.turn(45);
      state.turtle.move(cr, 700);

      cairo_surface_flush(state.surface);
      break;
    }

    free(event);
    xcb_flush(state.connection);
  }

  return 0;
}
