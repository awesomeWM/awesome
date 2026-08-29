local awful = { --DOC_HIDE --DOC_GEN_IMAGE --DOC_NO_USAGE
    popup = require("awful.popup"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {layoutlist = require("awful.widget.layoutlist")}, --DOC_HIDE
    layout = require("awful.layout") --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

    awful.popup {
        widget = awful.widget.layoutlist {
            source = awful.widget.layoutlist.source.default_layouts, --DOC_HIDE
            screen      = 1,
            base_layout = wibox.layout.flex.vertical
        },
        maximum_height = #awful.layout.layouts * 24,
        minimum_height = #awful.layout.layouts * 24,
        placement      = awful.placement.centered,
        hide_on_right_click = true,
    }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
