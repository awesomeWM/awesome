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

local wibox   = require( "wibox"         )
local surface = require( "gears.surface" )

-- If luacov is available, use it. Else, do nothing.
pcall(function()
    require("luacov.runner")(luacovpath)
end)

-- This is the main widget the tests will use as top level
local container = wibox.layout.fixed.vertical()

-- Let the test request a size and file format
local w, h, image_type = loadfile(file_path)(container)
image_type = image_type or "svg"

-- Emulate the event loop for 10 iterations
for _ = 1, 10 do
    awesome:emit_signal("refresh")
end

-- Get the example fallback size (the tests can return a size if the want)
local f_w, f_h = container:fit({dpi=96}, 9999, 9999)

-- There is an overhead that cause testbox "...", add 10 until someone
-- figures out the real equation
f_w, f_h = f_w+10, f_h+10

-- Save to the output file
local img = surface["widget_to_"..image_type](container, image_path.."."..image_type, w or f_w, h or f_h)
img:finish()

