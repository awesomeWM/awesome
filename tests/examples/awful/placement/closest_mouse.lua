--DOC_GEN_OUTPUT --DOC_GEN_IMAGE --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE

local c = client.gen_fake {x = 20, y = 20, width= 280, height=200, screen =screen[1]} --DOC_HIDE
local bw = c.border_width --DOC_HIDE

-- Left  --DOC_HIDE
mouse.coords {x=100,y=100} --DOC_HIDE
-- Move the mouse to the closest corner of the focused client
awful.placement.closest_corner(mouse, {include_sides=true, parent=c})
mouse.push_history() --DOC_HIDE
assert(mouse.coords().x == c.x) --DOC_HIDE
assert(mouse.coords().y == (c.y) + (c.height)/2+bw) --DOC_HIDE

-- Top left --DOC_HIDE
mouse.coords {x=60,y=40} --DOC_HIDE
awful.placement.closest_corner(mouse, {include_sides=true, parent=c}) --DOC_HIDE
mouse.push_history() --DOC_HIDE
assert(mouse.coords().x == c.x and mouse.coords().y == c.y) --DOC_HIDE

-- Top right --DOC_HIDE
mouse.coords {x=230,y=50} --DOC_HIDE
awful.placement.closest_corner(mouse, {include_sides=true, parent=c}) --DOC_HIDE
mouse.push_history() --DOC_HIDE
assert(mouse.coords().x == c.x+c.width+2*bw and mouse.coords().y == c.y) --DOC_HIDE

-- Right --DOC_HIDE
mouse.coords {x=240,y=140} --DOC_HIDE
awful.placement.closest_corner(mouse, {include_sides=true, parent=c}) --DOC_HIDE
mouse.push_history() --DOC_HIDE
assert(mouse.coords().x == c.x+c.width+2*bw and mouse.coords().y == c.y+c.height/2+bw) --DOC_HIDE

-- Bottom right --DOC_HIDE
mouse.coords {x=210,y=190} --DOC_HIDE
awful.placement.closest_corner(mouse, {include_sides=true, parent=c}) --DOC_HIDE
mouse.push_history() --DOC_HIDE
assert(mouse.coords().x == c.x+c.width+2*bw and mouse.coords().y == c.y+c.height+2*bw) --DOC_HIDE

-- Bottom --DOC_HIDE
mouse.coords {x=130,y=190} --DOC_HIDE
awful.placement.closest_corner(mouse, {include_sides=true, parent=c}) --DOC_HIDE
mouse.push_history() --DOC_HIDE
assert(mouse.coords().x == c.x+c.width/2+bw and mouse.coords().y == c.y + c.height + 2*bw) --DOC_HIDE

-- Top --DOC_HIDE
mouse.coords {x=130,y=30} --DOC_HIDE
awful.placement.closest_corner(mouse, {include_sides=true, parent=c}) --DOC_HIDE
mouse.push_history() --DOC_HIDE
assert(mouse.coords().x == c.x + c.width/2+bw and mouse.coords().y == c.y) --DOC_HIDE

-- Bottom left + outside of "c" --DOC_HIDE
mouse.coords {x=0,y=230} --DOC_HIDE
awful.placement.closest_corner(mouse, {include_sides=true, parent=c}) --DOC_HIDE
mouse.push_history() --DOC_HIDE
assert(mouse.coords().x == c.x and mouse.coords().y == c.y+c.height+2*bw) --DOC_HIDE

-- It is possible to emulate the mouse API to get the closest corner of
-- random area
local _, corner = awful.placement.closest_corner(
    {coords=function() return {x = 100, y=100} end},
    {include_sides = true, bounding_rect = {x=0, y=0, width=200, height=200}}
)
print("Closest corner:", corner)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
