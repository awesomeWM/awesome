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

local wibox     = require( "wibox"         )
local surface   = require( "gears.surface" )
local color     = require( "gears.color"   )
local beautiful = require( "beautiful"     )

-- If luacov is available, use it. Else, do nothing.
pcall(function()
    require("luacov.runner")(luacovpath)
end)

-- Silence debug warnings
require("gears.debug").print_warning = function() end

-- Create a generic rectangle widget to show layout disposition
local function generic_widget(text)
    return {
        {
            {
                draw = function(_, _, cr, width, height)
                    cr:set_source(color(beautiful.bg_normal))
                    cr:set_line_width(3)
                    cr:rectangle(0, 0, width, height)
                    cr:fill_preserve()
                    cr:set_source(color(beautiful.border_color))
                    cr:stroke()
                end,
                widget = wibox.widget.base.make_widget
            },
            text and {
                align  = "center",
                valign = "center",
                text   = text,
                widget = wibox.widget.textbox
            } or nil,
            widget = wibox.layout.stack
        },
        margins = 5,
        widget  = wibox.container.margin,
    }
end

-- Let the test request a size and file format
local widget, w, h = loadfile(file_path)(generic_widget)

-- Emulate the event loop for 10 iterations
for _ = 1, 10 do
    awesome:emit_signal("refresh")
end

-- Save to the output file
local img = surface["widget_to_svg"](widget, image_path..".svg", w or 200, h or 30)
img:finish()
