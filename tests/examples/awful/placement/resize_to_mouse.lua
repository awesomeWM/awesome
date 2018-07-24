--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local awful = {placement = require("awful.placement")}
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

screen._setup_grid(64, 48, {4, 4, 4, 4}, {workarea_sides=0})

local function test_touch_mouse(c)
    local coords = mouse.coords()

    return c:geometry().x == coords.x or c:geometry().y == coords.y
    or c:geometry().x+c:geometry().width+2*c.border_width == coords.x
    or c:geometry().y+c:geometry().height+2*c.border_width == coords.y
end

for s=1, 8 do
    local scr = screen[s]
    local x, y = scr.geometry.x, scr.geometry.y
    local c = client.gen_fake{x = x+22, y = y+16, width=20, height=15, screen=scr}
    assert(client.get()[s] == c)
end

for s=9, 16 do
    local scr = screen[s]
    local x, y = scr.geometry.x, scr.geometry.y
    local c = client.gen_fake{x = x+10, y = y+10, width=44, height=28, screen=scr}
    assert(client.get()[s] == c)
end

local function move_corsor(s, x, y)
    local sg = screen[s].geometry
    mouse.coords {x=sg.x+x,y=sg.y+y}
end

local all_coords_out = {
    { "top_left",     {10, 10}},
    { "top",          {32, 10}},
    { "top_right",    {60, 10}},
    { "right",        {60, 20}},
    { "bottom_right", {60, 40}},
    { "bottom",       {32, 40}},
    { "bottom_left",  {10, 40}},
    { "left",         {10, 29}},
}

local all_coords_in = {
    { "top_left",     {20, 18}},
    { "top",          {32, 18}},
    { "top_right",    {44, 18}},
    { "right",        {44, 24}},
    { "bottom_right", {44, 34}},
    { "bottom",       {32, 34}},
    { "bottom_left",  {20, 34}},
    { "left",         {32, 24}},
}

-- Top left
local s = 1
for _, v in ipairs(all_coords_out) do
    move_corsor(s, unpack(v[2]))
    assert(client.get()[s].screen == screen[s])
    awful.placement.resize_to_mouse(client.get()[s], {include_sides=true})
    mouse.push_history()
    assert(test_touch_mouse(client.get()[s]), v[1])
    s = s + 1
end

for _, v in ipairs(all_coords_in) do
    move_corsor(s, unpack(v[2]))
    assert(client.get()[s].screen == screen[s])
    awful.placement.resize_to_mouse(client.get()[s], {include_sides=true})
    mouse.push_history()
    assert(test_touch_mouse(client.get()[s]), v[1])
    s = s + 1
end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
