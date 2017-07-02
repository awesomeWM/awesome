local file_path, image_path = ...
require("_common_template")(...)
local wibox = require("wibox")

-- For the connections
require("naughty")

-- Create a screen
screen[1]._resize {x = 0, width = 800, height = 600}

-- Let the test request a size and file format
loadfile(file_path)()

-- Emulate the event loop for 10 iterations
for _ = 1, 10 do
    awesome:emit_signal("refresh")
end

local rect = {x1 = math.huge ,y1 = math.huge , x2 = -math.huge , y2 = -math.huge}

-- Get the region with wiboxes
for _, d in ipairs(drawin.get()) do
    local w = d.get_wibox and d:get_wibox() or nil
    if w then
        local geo = w:geometry()
        rect.x1 = math.min(rect.x1, geo.x                                )
        rect.y1 = math.min(rect.y1, geo.y                                )
        rect.x2 = math.max(rect.x2, geo.x + geo.width  + 2*w.border_width)
        rect.y2 = math.max(rect.y2, geo.y + geo.height + 2*w.border_width)
    end
end

-- Get rid of invalid drawins. The shims are very permissive and wont deny this.
if rect.x1 == rect.x2 or rect.y1 == rect.y2 then return end

local multi = wibox.layout {
    forced_width  = rect.x2 - rect.x1,
    forced_height = rect.y2 - rect.y1,
    layout        = wibox.layout.manual
}

-- Draw all normal wiboxes
for _, d in ipairs(drawin.get()) do
    local w = d.get_wibox and d:get_wibox() or nil
    if w then
        local geo = w:geometry()
        multi:add_at(w:to_widget(), {x = geo.x - rect.x1, y = geo.y - rect.y1})
    end
end

wibox.widget.draw_to_svg_file(
    multi, image_path..".svg", multi.forced_width, multi.forced_height
)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
