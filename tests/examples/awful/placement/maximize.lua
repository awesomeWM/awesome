--DOC_GEN_IMAGE --DOC_HIDE
screen[1]._resize {width = 128, height = 96} --DOC_HIDE
screen._add_screen {x = 140, y = 0, width = 128, height = 96} --DOC_HIDE
screen._add_screen {x = 280, y = 0, width = 128, height = 96} --DOC_HIDE
local placement = require("awful.placement") --DOC_HIDE

for k, pos in ipairs{ --DOC_HIDE
    "", "vertical", "horizontal" --DOC_HIDE
} do --DOC_HIDE
local c1 = client.gen_fake {--DOC_HIDE
    x = screen[k].geometry.x+40, --DOC_HIDE
    y = screen[k].geometry.y+40, width=75, height=50, screen=screen[k]} --DOC_HIDE
    placement.maximize(c1, {axis = pos ~= "" and pos or nil}) --DOC_HIDE

    if k == 1 then --DOC_HIDE
        assert(c1.width+2*c1.border_width == screen[1].geometry.width and --DOC_HIDE
               c1.height+2*c1.border_width == screen[1].geometry.height) --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
