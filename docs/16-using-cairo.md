# Using Cairo and LGI

The interface of Awesome is mostly based on a library called LGI. It provides
access to C libraries such as GTK, GLib, Cairo, Pango, PangoCairo and RSVG using
Lua code without having to write actual "glue" C code.

This is done using the GObject-introspection framework. The main advantage is
the time saved and large number of features exposed for free. The downside is
the lack of proper Lua centric documentation and examples. Some examples can be
found in [LGI's own documentation](https://github.com/pavouk/lgi/tree/master/docs),
but this does not directly explain how to use a concrete API.
Using other APIs requires some trial and error, and can be even impossible if
the introspection data is missing or inaccurate.
Using low-level APIs directly can easily cause crashes.
It is the programmer's responsibility to properly check return and error
values.

## Using LGI in Awesome

GObject and Gnome centric libraries tend to use the common C practice of
emulating namespaces using underscores in function names.
LGI exposes a proper namespace based API. For example, if the C function is:

    cairo_image_surface_create()

Then the LGI equivalent is:

    lgi.cairo.ImageSurface.create()

The same goes for enums:

    CAIRO_FORMAT_ARGB32

becomes:

    lgi.cairo.Format.ARGB32

LGI is also object oriented while the C API is function based. When those
functions take the "class" "object", then this:

    cairo_line_to(cr, x, y)

can be expressed as:

    cr:line_to(x, y)

It is however important to note some inconsistencies. For example, Cairo is
called `lgi.cairo` while GLib is called `lgi.GLib`. Figuring this out will
require some experimenting. The best way to do this without actually
reloading Awesome is to open the `lua` command in a terminal and use `print`:

    print("This will print a table address:", require("lgi").cairo)
    print("This will print an error:", require("lgi").Cairo)

It is recommended to avoid using `require` always when using a function, but
include the libraries at the top of your `rc.lua` or Lua module instead:

    local cairo = require("lgi").cairo

## The Cairo API

Cairo is a 2D graphic library used by Awesome, Gnome and XFCE. It allows to
e.g. paint paths on a `surface`.
Awesome uses it internally and being able to call it directly is a powerful
feature.

The following concepts are necessary to be able to use Cairo:

**Surface**:

A surface is the area where the painting will be done. There are multiple types
of surfaces including:

 * Color images with transparency (`ARGB32`) or without (`RGB24`)
 * Monochrome image surfaces with transparency (`A8`) or without (`A1`)
 * SVG vectorial surfaces
 * Native (XCB) surfaces
 * Framebuffers and other less interesting ones (from the point of
   view of Awesome)

For more details see [Surfaces](https://cairographics.org/manual/cairo-Image-Surfaces.html).

**Sources**:

Sources are elements like colors, patterns or gradients. See `gears.color` for
common sources.

For more details see [Pattern](http://cairographics.org/manual/cairo-cairo-pattern-t.html).

**Context and paths**:

A context is the proxy between the program and the surface, and holds a path.
Paths are something like a line, circle or rectangle, which may or
may not be closed (a shape).

All drawing operations on a surface are done via a context.
The current path is extended until it is used and reset (see next section).
Until then nothing will be drawn to the surface.  For example:

    cr:rectangle(0 , 0 , 10, 10)
    cr:rectangle(10, 10, 10, 10)

will not do anything until the operation is applied to the context.

For more details, read:

* [Path](http://cairographics.org/manual/cairo-Paths.html)
* [Context](http://cairographics.org/manual/cairo-cairo-t.html)
* [Transformation](http://cairographics.org/manual/cairo-Transformations.html)

A context also holds a transformation matrix (see `gears.matrix`), which is
used when applying an operation.

**Operations**:

Multiple operations can be done with the paths. The most common are:

 * *fill*: Fill the path with the current source.
 * *stroke*: Paint the path outline with the current source.
 * *mask*: Use the current source as an alpha mask while painting with the
   current operator.
 * *clip*: Crop the surface's workarea so nothing outside of the clip will be
   affected by all following operations.

**Operators**:

[Operators](http://cairographics.org/operators/) are modifiers used when
applying operations.

### Cairo in Awesome

The `wibox`es, `awful.wibar`s, `gears.wallpaper`s and
`awful.titlebar`s in Awesome contain Cairo surfaces, which can be accessed through
the `drawin` API.  This allows widgets to use the Cairo context directly.
See the
[declarative layout system](../documentation/03-declarative-layout.md.html)
and [new widgets](../documentation/04-new-widgets.md.html) articles for more
information and examples on how widgets work.

It is also possible to create surfaces manually. See `gears.surface` for
some examples. Here is the most simple example you can get:

    -- Create a surface
    local img = cairo.ImageSurface.create(cairo.Format.ARGB32, 50, 50)

    -- Create a context
    local cr  = cairo.Context(img)

    -- Set a red source
    cr:set_source(1, 0, 0)
    -- Alternative:
    cr:set_source(gears.color("#ff0000"))

    -- Add a 10px square path to the context at x=10, y=10
    cr:rectangle(10, 10, 10, 10)

    -- Actually draw the rectangle on img
    cr:fill()

This can then be used as `bgimage` for a `wibox`, `awful.wibar` or
`wibox.container.background`:

    screen.primary.mywibox.bgimage = img
