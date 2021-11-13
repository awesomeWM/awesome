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

for i=1, 5 do
    client.gen_fake{
        class = "client",
        name  = "Client #"..i,
        icon  = beautiful.awesome_icon,
        tags  = {t_real}
    }
end

client.get()[1].name = "Client 1# (floating)"
client.get()[2].name = "Client 2# (sticky)"
client.get()[3].name = "Client 3# (focus)"
client.get()[4].name = "Client 4# (urgent)"
client.get()[5].name = "Client 5# (minimized)"

client.get()[1].floating = true
client.get()[2].sticky = true
client.get()[4].urgent = true
client.get()[5].minimized = true
client.focus = client.get()[3]

--DOC_HIDE_END
for _, col in ipairs { "#ff0000", "#00ff00", "#0000ff" } do
    beautiful.tasklist_fg_minimize = col

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
