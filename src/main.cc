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
#include <chrono>
#include <thread>

#include <cstdint>
#include <cmath>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_keysyms.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#include "turtle.hh"
#include "config.hh"

using namespace std::chrono_literals;

// The width and height of the window in pixels.
// TODO: Make this configurable.
const uint16_t WIDTH = 500;
const uint16_t HEIGHT = 500;

struct State {
  Turtle turtle;

  xcb_connection_t *connection;
  xcb_screen_t *screen;
  xcb_window_t window;
  cairo_surface_t *surface;
  cairo_t *cr;

  State() {
    // NOTE: This method is not for the faint of heart. X protocol/xcb horrors
    // lie below. I hope that this serves as a sufficient warning for my future
    // self.

    // The screen that the server prefers. On modern interactive desktops,
    // there typically is only 1 screen shared amongst the displays with XRANDR
    // or similar, and thus this is commonly the single screen #0.
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

    uint32_t mask = XCB_CW_EVENT_MASK;
    uint32_t values[] = {
      XCB_EVENT_MASK_STRUCTURE_NOTIFY |
      XCB_EVENT_MASK_EXPOSURE |
      XCB_EVENT_MASK_KEY_PRESS
    };
    this->window = xcb_generate_id(this->connection);
    xcb_create_window(
      this->connection,
      XCB_COPY_FROM_PARENT,          // Depth
      this->window,                  // ID
      this->screen->root,            // Parent window
      0, 0,                          // Position (x, y)
      WIDTH, HEIGHT,                 // Size
      2,                             // Border width
      XCB_WINDOW_CLASS_INPUT_OUTPUT, // Class
      XCB_COPY_FROM_PARENT,          // Visual ID
      mask, values                   // Mask
    );

    // Register for the ICCM `WM_DELETE_WINDOW` ClientMessage event.
    // https://x.org/releases/current/doc/xorg-docs/icccm/icccm.html#Window_Deletion
    auto protocols_cookie =
      xcb_intern_atom(this->connection, 0, 12, "WM_PROTOCOLS");
    auto *protocols_reply =
      xcb_intern_atom_reply(this->connection, protocols_cookie, 0);
    auto delete_cookie = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
    auto *delete_reply = xcb_intern_atom_reply(connection, delete_cookie, 0);
    xcb_change_property(
      this->connection,
      XCB_PROP_MODE_REPLACE, // Mode
      this->window,          // Window
      protocols_reply->atom, // Property
      XCB_ATOM_ATOM,         // Type
      32,                    // Format
      1,                     // Data length
      &delete_reply->atom    // Data
    );
    free(delete_reply);
    free(protocols_reply);

    auto visual_type = xcb_aux_find_visual_by_id(
      this->screen, this->screen->root_visual
    );
    this->surface = cairo_xcb_surface_create(
      this->connection, this->window, visual_type, WIDTH, HEIGHT
    );
    this->cr = cairo_create(this->surface);
  }

  ~State() {
    cairo_destroy(this->cr);
    cairo_surface_finish(this->surface);
    xcb_disconnect(this->connection);
  }
};

bool handle_xcb_event(xcb_generic_event_t *event, State& state) {
  if (!event) {
    // TODO: Check the exit code and present a more granular error message.
    if (xcb_connection_has_error(state.connection)) {
      spdlog::critical("Connection to X server lost");
      exit(1);
    }

    return false;
  }

  // NOTE: Remember to register for the events in `xcb_create_window` when
  // adding new cases here.
  switch (event->response_type & ~0x80) {
  case XCB_EXPOSE: {
    spdlog::debug("Expose event recieved");
    auto expose = (xcb_expose_event_t *)event;

    // Avoid extra redraws by checking if this is the last expose event in
    // the sequence.
    if (expose->count != 0) {
      break;
    }

    auto& turtle = state.turtle;

    // Background
    // TODO: Make this configurable.
    cairo_set_source_rgb(state.cr, 1, 1, 1);
    cairo_paint(state.cr);

    // Test the basic turtle commands.
    cairo_set_line_width(state.cr, 3);
    cairo_set_source_rgb(state.cr, 0, 0, 0);
    turtle.reset();
    turtle.turn(45);
    turtle.move(state.cr, 700);

    // TODO: Ascertain whether this call is really needed.
    cairo_surface_flush(state.surface);
    break;
  }

  case XCB_CLIENT_MESSAGE: {
    spdlog::debug("ClientMessage event recieved");
    auto client_message = (xcb_client_message_event_t *)event;

    auto delete_cookie =
      xcb_intern_atom(state.connection, 0, 16, "WM_DELETE_WINDOW");
    auto *delete_reply =
      xcb_intern_atom_reply(state.connection, delete_cookie, 0);

    if (client_message->data.data32[0] == delete_reply->atom) {
      free(delete_reply);
      return true;
    }

    free(delete_reply);
    break;
  }

  case XCB_CONFIGURE_NOTIFY: {
    spdlog::debug("ConfigureNotify event recieved");
    auto configure = (xcb_configure_notify_event_t *)event;

    cairo_xcb_surface_set_size(state.surface, configure->width, configure->height);

    cairo_surface_flush(state.surface);
    break;
  }

  case XCB_KEY_PRESS: {
    spdlog::debug("KeyPress event recieved");
    auto key_press = (xcb_key_press_event_t *)event;

    auto keysyms = xcb_key_symbols_alloc(state.connection);
    auto keysym = xcb_key_press_lookup_keysym(keysyms, key_press, 0);
    free(keysyms);

    // Quit if q was pressed.
    if (keysym == 113) {
      return true;
    }

    break;
  }

  default:
    // Ignore unknown event types.
    break;
  }

  return false;
}

int run() {
  // Initialise state such as the server connection.
  State state;

  // Make the window visible.
  xcb_map_window(state.connection, state.window);
  xcb_flush(state.connection);

  using clock = std::chrono::steady_clock;

  // Use a fixed timestep of (1s)/(60 fps) = 16 ms.
  constexpr std::chrono::nanoseconds TIMESTEP = 16ms;

  spdlog::debug("Starting event loop...");
  bool done = false;
  while (!done) {
    auto start = clock::now();

    // Process input via X events over xcb.
    auto *event = xcb_poll_for_event(state.connection);
    if (handle_xcb_event(event, state)) {
      done = true;
    }
    free(event);

    xcb_flush(state.connection);

    auto end = clock::now();
    std::this_thread::sleep_for(start + TIMESTEP - end);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  CLI::App app;
  CLI::Option *version_flag = app.add_flag(
    "-V,--version",
    "Print version information and exit"
  );
  CLI::Option *verbose_flag = app.add_flag(
    "-v,--verbose",
    "Enable verbose output"
  );

  CLI11_PARSE(app, argc, argv);

  if (version_flag->count() > 0) {
    std::cout << "xturtle " << XTURTLE_VERSION << " ";
#ifdef XTURTLE_DEBUG_ENABLED
    std::cout << "(debug enabled)";
#endif
    std::cout << "\n";

    return 0;
  }

  if (verbose_flag->count() > 0) {
#ifdef XTURTLE_DEBUG_ENABLED
    spdlog::set_level(spdlog::level::debug);
#elif
    spdlog::set_level(spdlog::level::info);
#endif
  } else {
    spdlog::set_level(spdlog::level::warn);
  }

  return run();
}
