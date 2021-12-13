local file_path, image_path = ...
require("_common_template")(...)

local wibox   = require( "wibox"         )
local surface = require( "gears.surface" )
local cairo = require("lgi").cairo
local color = require("gears.color")
local beautiful = require("beautiful")

--- Create a copy of the widget frozen in time.
-- This is useful whe the global state is modified between the time this is
-- called and the time the final output is rendered.
function _memento(wdg, width, height, context, force) -- luacheck: globals _memento
    context = context or {dpi=96}

    local w, h = wdg:fit(context, width or 9999, height or 9999)

    w, h = math.min(force and width or w,  width or 9999), math.min(force and height or h, height or 9999)

    local memento = cairo.RecordingSurface(
        cairo.Content.CAIRO_FORMAT_ARGB32,
        cairo.Rectangle { x = 0, y = 0, width = w, height = h }
    )

    local cr = cairo.Context(memento)

    cr:set_source(color(beautiful.fg_normal))

    wibox.widget.draw_to_cairo_context(wdg, cr, w, h, context)

    return wibox.widget {
        fit = function()
            return w, h
        end,
        draw = function(_, _, cr2)
            cr2:set_source_surface(memento)
            cr2:paint()
        end
    }
end

-- This is the main widget the tests will use as top level
local container = wibox.layout.fixed.vertical()

-- Let the test request a size and file format
local w, h, image_type = loadfile(file_path)(container)
image_type = image_type or "svg"

-- Emulate the event loop for 10 iterations
for _ = 1, 10 do
    require("gears.timer").run_delayed_calls_now()
end

-- Get the example fallback size (the tests can return a size if the want)
local f_w, f_h = container:fit({dpi=96}, 9999, 9999)

-- There is an overhead that cause testbox "...", add 10 until someone
-- figures out the real equation
f_w, f_h = f_w+10, f_h+10

-- Save to the output file
local img = surface["widget_to_"..image_type](container, image_path.."."..image_type, w or f_w, h or f_h)
img:finish()

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
