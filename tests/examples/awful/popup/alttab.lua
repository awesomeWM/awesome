--DOC_GEN_IMAGE --DOC_HIDE
local awful = { --DOC_HIDE --DOC_NO_USAGE
    popup = require("awful.popup"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {clienticon =require("awful.widget.clienticon"), --DOC_HIDE
              tasklist = require("awful.widget.tasklist")} --DOC_HIDE
} --DOC_HIDE
local gears = { shape = require("gears.shape") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

for _=1, 6 do --DOC_HIDE
    local c = client.gen_fake {x = 80, y = 55, width=75, height=50} --DOC_HIDE
    c.icon = beautiful.awesome_icon --DOC_HIDE
    c.minimized = true --DOC_HIDE
end --DOC_HIDE

local tasklist_buttons = nil --DOC_HIDE

    awful.popup {
        widget = awful.widget.tasklist {
            screen   = screen[1],
            filter   = awful.widget.tasklist.filter.allscreen,
            buttons  = tasklist_buttons,
            style    = {
                shape = gears.shape.rounded_rect,
            },
            layout   = {
                spacing = 5,
                row_count = 2,
                layout = wibox.layout.grid.horizontal
            },

            widget_template = {
                {
                    {
                        id     = "clienticon",
                        widget = awful.widget.clienticon,
                    },
                    margins = 4,
                    widget  = wibox.container.margin,
                },
                id              = "background_role",
                forced_width    = 48,
                forced_height   = 48,
                widget          = wibox.container.background,
                create_callback = function(self, c, index, objects) --luacheck: no unused
                    self:get_children_by_id("clienticon")[1].client = c
                end,
            },
        },
        border_color = "#777777",
        border_width = 2,
        ontop        = true,
        placement    = awful.placement.centered,
        shape        = gears.shape.rounded_rect
    }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
