local file_path, image_path = ...
require("_common_template")(...)

local wibox   = require( "wibox"         )
local surface = require( "gears.surface" )

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
