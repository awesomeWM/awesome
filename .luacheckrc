-- Only allow symbols available in all Lua versions
std = "min"

-- Get rid of "unused argument self"-warnings
self = false

-- The unit tests can use busted
files["spec"].std = "+busted"

-- The default config may set global variables
files["awesomerc.lua"].allow_defined_top = true

-- Global objects defined by the C code
read_globals = {
    "awesome",
    "button",
    "client",
    "dbus",
    "drawable",
    "drawin",
    "key",
    "keygrabber",
    "mouse",
    "mousegrabber",
    "root",
    "selection",
    "tag",
    "window",
}

-- screen may not be read-only, because newer luacheck versions complain about
-- screen[1].tags[1].selected = true.
-- client may not be read-only due to client.focus.
globals = {
    "screen",
    "client"
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
