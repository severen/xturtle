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

#ifndef XTURTLE_TURTLE_H
#define XTURTLE_TURTLE_H

#include <cairo/cairo.h>

class Turtle {
  private:
    struct Pen {
      double red;
      double green;
      double blue;
      double thickness;
      bool down;

      Pen();
    };

    Pen pen;
    double x = 0;
    double y = 0;
    double direction = 0;

  public:
    void pen_up();
    void pen_down();

    void turn(double degrees);
    void move(cairo_t *cr, double distance);
    void reset();

    void set_pen_color(double red, double green, double blue);
    void set_pen_thickness(double thickness);
};

#endif // XTURTLE_TURTLE_H
