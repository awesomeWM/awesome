--DOC_GEN_IMAGE --DOC_HIDE
screen[1]._resize {x = 175, width = 128, height = 96} --DOC_NO_USAGE --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
local c = client.gen_fake {x = 220, y = 35, width=40, height=30} --DOC_HIDE

    -- Simulate Windows 7 "edge snap" (also called aero snap) feature
    local axis = "vertically"

    local f = awful.placement.scale
        + awful.placement.left
        + (axis and awful.placement["maximize_"..axis] or nil)

    local geo = f(client.focus, {honor_workarea=true, to_percent = 0.5})

local wa = screen[1].workarea--DOC_HIDE
assert(c.x == wa.x and geo.x == wa.x)--DOC_HIDE
assert(c.y == wa.y) --DOC_HIDE
assert(c.width  == wa.width/2 - 2*c.border_width)--DOC_HIDE
assert(c.height == wa.height  - 2*c.border_width)--DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
