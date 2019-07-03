--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local awful = { --DOC_HIDE
    tag = require("awful.tag"), --DOC_HIDE
    layout = require("awful.layout"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {taglist = require("awful.widget.taglist")} --DOC_HIDE
} --DOC_HIDE
local gears = { shape = require("gears.shape") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local s = screen[1] --DOC_HIDE
local taglist_buttons = nil -- To make luacheck shut up --DOC_HIDE

local tags = awful.tag({ "term", "net", "mail", "chat", "files" }, --DOC_HIDE
    s, awful.layout.suit.floating) --DOC_HIDE

for i=1, 5 do tags[i].selected = false end --DOC_HIDE
tags[2].selected = true --DOC_HIDE

--DOC_HIDE add some clients to some tags
local c = client.gen_fake {x = 80, y = 55, width=75, height=50} --DOC_HIDE
local c2 = client.gen_fake {x = 80, y = 55, width=75, height=50} --DOC_HIDE
c:tags(tags[4]) --DOC_HIDE
c2:tags(tags[1]) --DOC_HIDE

    s.mytaglist = awful.widget.taglist {
        screen  = s,
        filter  = awful.widget.taglist.filter.all,
        style   = {
            shape = gears.shape.powerline
        },
        layout   = {
            spacing = -12,
            spacing_widget = {
                color  = "#dddddd",
                shape  = gears.shape.powerline,
                widget = wibox.widget.separator,
            },
            layout  = wibox.layout.fixed.horizontal
        },
        widget_template = {
            {
                {
                    {
                        {
                            {
                                id     = "index_role",
                                widget = wibox.widget.textbox,
                            },
                            margins = 4,
                            widget  = wibox.container.margin,
                        },
                        bg     = "#dddddd",
                        shape  = gears.shape.circle,
                        widget = wibox.container.background,
                    },
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
                left  = 18,
                right = 18,
                widget = wibox.container.margin
            },
            id     = "background_role",
            widget = wibox.container.background,

            -- Add support for hover colors and an index label
            create_callback = function(self, c3, index, objects) --luacheck: no unused args
                self:get_children_by_id("index_role")[1].markup = "<b> "..c3.index.." </b>"

                self:connect_signal("mouse::enter", function()
                    if self.bg ~= "#ff0000" then
                        self.backup     = self.bg
                        self.has_backup = true
                    end
                    self.bg = "#ff0000"
                end)

                self:connect_signal("mouse::leave", function()
                    if self.has_backup then self.bg = self.backup end
                end)
            end,
            update_callback = function(self, c3, index, objects) --luacheck: no unused args
                self:get_children_by_id("index_role")[1].markup = "<b> "..c3.index.." </b>"
            end,
        },
        buttons = taglist_buttons
    }

s.mytaglist.forced_width = 400 --DOC_HIDE
s.mytaglist.forced_height = 18 --DOC_HIDE
s.mytaglist._do_taglist_update_now() --DOC_HIDE

parent:add(wibox.widget.background(s.mytaglist, beautiful.bg_normal)) --DOC_HIDE
