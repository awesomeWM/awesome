local file_path, image_path, luacovpath = ...   

-- Set the global shims
-- luacheck: globals awesome client tag drawin screen
awesome = require( "awesome" )
client  = require( "client"  )
tag     = require( "tag"     )
drawin  = require( "drawin"  )
screen  = require( "screen"  )

-- Force luacheck to be silent about setting those as unused globals
assert(awesome and client and tag)

local beautiful = require( "beautiful"     )
local wibox     = require( "wibox"         )
local surface   = require( "gears.surface" )
local shape     = require( "gears.shape"   )

-- If luacov is available, use it. Else, do nothing.
pcall(function()
    require("luacov.runner")(luacovpath)
end)

-- Silence debug warnings
require("gears.debug").print_warning = function() end

-- Let the test request a size and file format
local before, after = loadfile(file_path)()

local container = wibox.widget {
    {
        {
            {
                before,
                shape_border_color = beautiful.border_color,
                shape_border_width = beautiful.border_width,
                shape              = shape.rounded_rect,
                widget             = wibox.container.background,
            },
            strategy = 'exact',
            width    = 70,
            height   = 40,
            widget   = wibox.container.constraint
        },
        {
            {
                {
                    text   = " ",
                    widget = wibox.widget.textbox,
                },
                bg                 = beautiful.bg_normal,
                shape_border_color = beautiful.border_color,
                shape_border_width = beautiful.border_width,
                widget             = wibox.container.background,
                shape              = shape.transform(shape.arrow)
                                        : rotate_at(15,15,math.pi/2)
                                        : translate(0,-8)
                                        : scale(0.9, 0.9),
            },
            strategy = 'exact',
            width    = 42,
            height   = 40,
            widget   = wibox.container.constraint
        },
        {
            {
                after,
                shape_border_color = beautiful.border_color,
                shape_border_width = beautiful.border_width,
                shape              = shape.rounded_rect,
                widget             = wibox.container.background,
            },
            strategy = 'exact',
            width    = 70,
            height   = 40,
            widget   = wibox.container.constraint
        },
        layout = wibox.layout.align.horizontal
    },
    margins = 10,
    widget  = wibox.container.margin,
}

-- Emulate the event loop for 10 iterations
for _ = 1, 10 do
    awesome:emit_signal("refresh")
end

-- Get the example fallback size (the tests can return a size if the want)
local f_w, f_h = container:fit({dpi=96}, 9999, 9999)

-- Save to the output file
local img = surface.widget_to_svg(container, image_path..".svg", f_w, f_h)
img:finish()

