--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local awful = { --DOC_HIDE
    tag = require("awful.tag"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {clienticon =require("awful.widget.clienticon"), --DOC_HIDE
              tasklist = require("awful.widget.tasklist")} --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local s = screen[1] --DOC_HIDE
local tasklist_buttons = nil -- To make luacheck shut up --DOC_HIDE

local t_real = awful.tag.add("Test", {screen=screen[1]}) --DOC_HIDE

for i=1, 3 do --DOC_HIDE
    local c = client.gen_fake {x = 80, y = 55, width=75, height=50} --DOC_HIDE
    c:tags{t_real} --DOC_HIDE
    c.icon = beautiful.awesome_icon --DOC_HIDE
    c.name = " Client "..i.."  " --DOC_HIDE
    client.focus = i==2 and c or client.focus --DOC_HIDE
end --DOC_HIDE

    s.mytasklist = awful.widget.tasklist {
        screen   = s,
        filter   = awful.widget.tasklist.filter.currenttags,
        buttons  = tasklist_buttons,
        layout   = {
            spacing_widget = {
                {
                    forced_width  = 5,
                    forced_height = 24,
                    thickness     = 1,
                    color         = "#777777",
                    widget        = wibox.widget.separator
                },
                valign = "center",
                halign = "center",
                widget = wibox.container.place,
            },
            spacing = 1,
            layout  = wibox.layout.fixed.horizontal
        },
        -- Notice that there is *NO* `wibox.wibox` prefix, it is a template,
        -- not a widget instance.
        widget_template = {
            {
                wibox.widget.base.make_widget(),
                forced_height = 5,
                id            = "background_role",
                widget        = wibox.container.background,
            },
            {
                {
                    id     = "clienticon",
                    widget = awful.widget.clienticon,
                },
                margins = 5,
                widget  = wibox.container.margin
            },
            nil,
            create_callback = function(self, c, index, objects) --luacheck: no unused args
                self:get_children_by_id("clienticon")[1].client = c
            end,
            layout = wibox.layout.align.vertical,
        },
    }

s.mytasklist.forced_width = 400 --DOC_HIDE
s.mytasklist.forced_height = 48 --DOC_HIDE
s.mytasklist._do_tasklist_update_now() --DOC_HIDE

parent:add( s.mytasklist) --DOC_HIDE
