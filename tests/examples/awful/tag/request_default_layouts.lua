--DOC_ASTERISK --DOC_NO_USAGE --DOC_HIDE_START
local awful     = require("awful")

--DOC_HIDE_END

    tag.connect_signal("request::default_layouts", function()
        awful.layout.append_default_layouts({
            awful.layout.suit.floating,
            awful.layout.suit.tile,
            awful.layout.suit.tile.left,
            awful.layout.suit.tile.bottom,
            awful.layout.suit.tile.top,
            awful.layout.suit.fair,
            awful.layout.suit.fair.horizontal,
            awful.layout.suit.spiral,
            awful.layout.suit.spiral.dwindle,
            awful.layout.suit.max,
            awful.layout.suit.max.fullscreen,
            awful.layout.suit.magnifier,
            awful.layout.suit.corner.nw,
        })
    end)

--DOC_HIDE_START
tag.emit_signal("request::default_layouts")

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
