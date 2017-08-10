local parent = ... --DOC_NO_USAGE --DOC_HIDE
local awful = { --DOC_HIDE
    tag = require("awful.tag"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {tasklist = require("awful.widget.tasklist")} --DOC_HIDE
} --DOC_HIDE
local gears = { shape = require("gears.shape") } --DOC_HIDE
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
end --DOC_HIDE

    s.mytasklist = awful.widget.tasklist {
        screen   = s,
        filter   = awful.widget.tasklist.filter.currenttags,
        buttons  = tasklist_buttons,
        style    = {
            shape_border_width = 1,
            shape_border_color = "#777777",
            shape  = gears.shape.rounded_bar,
        },
        layout   = {
            spacing = 10,
            spacing_widget = {
                {
                    forced_width = 5,
                    shape        = gears.shape.circle,
                    widget       = wibox.widget.separator
                },
                valign = "center",
                halign = "center",
                widget = wibox.container.place,
            },
            layout  = wibox.layout.flex.horizontal
        },
        -- Notice that there is *NO* `wibox.wibox` prefix, it is a template,
        -- not a widget instance.
        widget_template = {
            {
                {
                    {
                        {
                            id     = "icon_role",
                            widget = wibox.widget.imagebox,
                        },
                        margins = 2,
                        widget  = wibox.container.margin,
                    },
                    {
                        id     = "text_role",
                        widget = wibox.widget.textbox,
                    },
                    layout = wibox.layout.fixed.horizontal,
                },
                left  = 10,
                right = 10,
                widget = wibox.container.margin
            },
            id     = "background_role",
            widget = wibox.container.background,
        },
    }

s.mytasklist.forced_width = 400 --DOC_HIDE
s.mytasklist.forced_height = 18 --DOC_HIDE
s.mytasklist._do_tasklist_update_now() --DOC_HIDE

parent:add( s.mytasklist) --DOC_HIDE
