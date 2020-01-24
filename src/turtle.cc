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

#include <cmath>

#include "turtle.hh"
#include "util.hh"

Turtle::Pen::Pen() : red(0), green(0), blue(0), thickness(0.5), down(true) {}

void Turtle::pen_up() {
  this->pen.down = false;
}

void Turtle::pen_down() {
  this->pen.down = true;
}

void Turtle::turn(double degrees) {
  this->direction += M_PI / 180.0 * degrees;
}

void Turtle::move(cairo_t *cr, double distance) {
  double new_x = this->x + distance * cos(this->direction);
  double new_y = this->y + distance * sin(this->direction);

  if (this->pen.down) {
    draw_line(cr, this->x, this->y, new_x, new_y);
  }

  this->x = new_x;
  this->y = new_y;
}

void Turtle::reset() {
  this->x = 0;
  this->x = 0;
  this->direction = 0;
  this->pen = Pen();
}

void Turtle::set_pen_color(double red, double green, double blue) {
  this->pen.red = red;
  this->pen.green = green;
  this->pen.blue = blue;
}

void Turtle::set_pen_thickness(double thickness) {
  this->pen.thickness = thickness;
}
