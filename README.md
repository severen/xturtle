# xturtle

xturtle is a toy [turtle graphics](https://en.wikipedia.org/wiki/Turtle_graphics)
program for Linux/X11 that uses GNU Guile for scripting.

This is mostly an exercise in learning how X11 works and how to write programs
for it with libraries such as XCB and Cairo. It also serves as an excuse to get
better at writing C/C++.

## Building

To build xturtle, the [Meson](https://mesonbuild.com/) build system must be
installed, as well as the following dependencies:

- xcb;
- xcb-util;
- cairo;
- guile-2.2;
- spdlog (optional, will be built from source if not available).

If the above are available, run the following to build xturtle:

```sh
$ meson build
$ ninja -C build
```

The compiled `xturtle` binary will be in the `build` directory.
