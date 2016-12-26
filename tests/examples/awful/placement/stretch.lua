--DOC_HIDE_ALL
screen[1]._resize {width = 128, height = 96}
screen._add_screen {x = 140, y = 0  , width = 128, height = 96}
screen._add_screen {x = 0  , y = 110, width = 128, height = 96}
screen._add_screen {x = 140, y = 110, width = 128, height = 96}
local placement = require("awful.placement")

for k, pos in ipairs{
    "up", "down", "left", "right"
} do
local c1 = client.gen_fake {--DOC_HIDE
    x = screen[k].geometry.x+20,
    y = screen[k].geometry.y+20, width=75, height=50, screen=screen[k]}

    placement.stretch(c1, {direction=pos})
end
