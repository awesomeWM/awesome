--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local placement = require("awful.placement")
screen[1]._resize {width = 180, height = 120} --DOC_HIDE
screen._add_screen {x = 190, y = 0, width = 180, height = 120} --DOC_HIDE
screen._add_screen {x = 380, y = 0, width = 180, height = 120} --DOC_HIDE

for s in screen do
    require("awful.tag").add("1", {screen=s, selected=true}) --DOC_HIDE
end

for _, pos in ipairs{"left", "right"} do
    local c1 = client.gen_fake {x = 80, y = 55, width=78, height=50}
    placement.align(client.focus, {position = pos, honor_workarea=true})
    c1:_hide_all()
    placement.maximize_vertically(client.focus, {position = pos, honor_workarea=true})
    c1:set_label(pos)
end

for _, pos in ipairs{"top", "bottom"} do
    local c1 = client.gen_fake {x = 80, y = 55, width=75, height=48,screen=screen[2]}
    placement.align(client.focus, {position = pos, honor_workarea=true})
    c1:_hide_all()
    placement.maximize_horizontally(client.focus, {position = pos, honor_workarea=true})
    c1:set_label(pos)
end

for _, pos in ipairs{"top_left", "top_right", "bottom_left", "bottom_right"} do
    local c1 = client.gen_fake {x = 280, y = 55, width=79, height=48, screen=screen[3]}
    c1:_hide_all()
    placement.align(client.focus, {position = pos, honor_workarea=true})
    c1:set_label(pos)
end

return {hide_lines=true}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
