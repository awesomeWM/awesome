--DOC_HIDE_START
--DOC_GEN_IMAGE
local parent    = ...
local beautiful = require("beautiful")
local wibox = require("wibox")
local awful     = { widget = { tasklist = require("awful.widget.tasklist") } }
local t_real = require("awful.tag").add("Test", {screen=screen[1]})

local s = screen[1]
parent.spacing = 5
beautiful.tasklist_fg_focus = "#000000"

for i=1, 3 do
    client.gen_fake{
        class = "client",
        name  = "Client #"..i,
        icon  = beautiful.awesome_icon,
        tags  = {t_real}
    }
end

--DOC_HIDE_END
for _, value in ipairs { true, false } do
    beautiful.tasklist_disable_task_name = value

    --DOC_HIDE_START
    local tasklist = awful.widget.tasklist {
        screen = s,
        filter = awful.widget.tasklist.filter.currenttags,
        layout = wibox.layout.fixed.horizontal
    }

    tasklist._do_tasklist_update_now()
    parent:add(_memento(tasklist, nil, 22)) --luacheck: globals _memento
    --DOC_HIDE_END
end


--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
