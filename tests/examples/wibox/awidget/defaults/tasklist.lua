--DOC_GEN_IMAGE --DOC_HIDE_ALL
local parent = ... --DOC_NO_USAGE
local awful = {
    tag = require("awful.tag"),
    placement = require("awful.placement"),
    widget = {tasklist = require("awful.widget.tasklist")}
}
local beautiful = require("beautiful")

local s = screen[1]
local tasklist_buttons = nil -- To make luacheck shut up

local t_real = awful.tag.add("Test", {screen=screen[1]})

for i=1, 3 do
    local c = client.gen_fake {x = 80, y = 55, width=75, height=50}
    c:tags{t_real}
    c.icon = beautiful.awesome_icon
    c.name = " Client "..i.."  "
end

    s.mytasklist = awful.widget.tasklist {
        screen   = s,
        filter   = awful.widget.tasklist.filter.currenttags,
        buttons  = tasklist_buttons,
    }

s.mytasklist.forced_width = 200
s.mytasklist.forced_height = 18
s.mytasklist._do_tasklist_update_now()

parent:add( s.mytasklist)
