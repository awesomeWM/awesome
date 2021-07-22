--DOC_GEN_IMAGE --DOC_HEADER --DOC_HIDE
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local awful = {--DOC_HIDE
    widget = {layoutlist = require("awful.widget.layoutlist")}, --DOC_HIDE
    layout = require("awful.layout") --DOC_HIDE
}--DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

local ll = --DOC_HIDE

        awful.widget.layoutlist {
            base_layout = wibox.layout.fixed.horizontal,
            style = {
                disable_name = true,
                spacing      = 3,
            },
            source = function() return {
                awful.layout.suit.floating,
                awful.layout.suit.tile,
                awful.layout.suit.tile.left,
                awful.layout.suit.tile.bottom,
                awful.layout.suit.tile.top,
            } end,
            screen      = 1,
        }

ll.forced_height = 24 --DOC_HIDE
ll.forced_width = 100 --DOC_HIDE
parent:add(ll) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
