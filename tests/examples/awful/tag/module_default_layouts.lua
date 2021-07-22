--DOC_ASTERISK --DOC_NO_USAGE --DOC_HIDE_START
local awful     = require("awful")
local custom_module = {
    layout_1 = awful.layout.suit.floating,
    layout_2 = awful.layout.suit.tile,
}

--DOC_HIDE_END

    tag.connect_signal("request::default_layouts", function()
        awful.layout.append_default_layouts({
            custom_module.layout_1,
            custom_module.layout_2,
        })
    end)

--DOC_HIDE_START
tag.emit_signal("request::default_layouts")

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
