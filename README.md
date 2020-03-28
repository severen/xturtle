# xturtle

xturtle is a [turtle graphics](https://en.wikipedia.org/wiki/Turtle_graphics)
program for Linux/X11 that is controlled by
[Scheme](https://en.wikipedia.org/wiki/Scheme_(programming_language)), a
dialect of the Lisp family of programming languages.

**Warning**: xturtle was written for my own personal education in bare-bones
Linux application development and as such should not be used for anything
serious. That being said, it may serve as a useful example of wiring up XCB and
Cairo.

## Building

To build xturtle, the [Meson](https://mesonbuild.com/) build system must be
installed, as well as the following dependencies:

- xcb;
- xcb-util;
- xcb-util-wm;
- xcb-keysyms;
- cairo;
- guile-2.2;
- spdlog (optional, will be built from source if not available);
- CLI11 (optional, will be built from source if not available).

If the above are available, run the following to build xturtle:
```sh
$ meson build
$ ninja -C build
```
