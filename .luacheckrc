-- Only allow symbols available in all Lua versions
std = "min"

-- Get rid of "unused argument self"-warnings
self = false

-- The unit tests can use busted
files["spec"].std = "+busted"

-- The default config may set global variables
files["awesomerc.lua"].allow_defined_top = true

-- This file itself
files[".luacheckrc"].ignore = {"111", "112", "131"}

-- Theme files, ignore max line length
files["themes/*"].ignore = {"631"}

-- Global objects defined by the C code
read_globals = {
    "awesome",
    "button",
    "dbus",
    "drawable",
    "drawin",
    "key",
    "keygrabber",
    "mousegrabber",
    "root",
    "selection",
    "tag",
    "window",
    "table.unpack",
    "math.atan2",
}

-- screen may not be read-only, because newer luacheck versions complain about
-- screen[1].tags[1].selected = true.
-- The same happens with the following code:
--   local tags = mouse.screen.tags
--   tags[7].index = 4
-- client may not be read-only due to client.focus.
globals = {
    "screen",
    "mouse",
    "client"
}

-- Enable cache (uses .luacheckcache relative to this rc file).
cache = true

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
