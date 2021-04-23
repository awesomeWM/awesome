--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local awful = { --DOC_HIDE
    tag = require("awful.tag"), --DOC_HIDE
    layout = require("awful.layout"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {taglist = require("awful.widget.taglist")} --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local s = screen[1] --DOC_HIDE
local taglist_buttons = nil -- To make luacheck shut up --DOC_HIDE

local tags = {} --DOC_HIDE

table.insert(tags, --DOC_HIDE
    awful.tag.add("one", {})
) --DOC_HIDE

--DOC_NEWLINE

table.insert(tags, --DOC_HIDE
    awful.tag.add("two", {
        icon = beautiful.awesome_icon
    })
) --DOC_HIDE

--DOC_NEWLINE

table.insert(tags, --DOC_HIDE
    awful.tag.add("three", {})
) --DOC_HIDE

--DOC_NEWLINE

for i=1, 3 do tags[i].selected = false end --DOC_HIDE
tags[2].selected = true --DOC_HIDE

--DOC_HIDE add some clients to some tags
local c = client.gen_fake {x = 80, y = 55, width=75, height=50} --DOC_HIDE
local c2 = client.gen_fake {x = 80, y = 55, width=75, height=50} --DOC_HIDE
c:tags(tags[3]) --DOC_HIDE
c2:tags(tags[1]) --DOC_HIDE

s.mytaglist = awful.widget.taglist { --DOC_HIDE
    screen  = s, --DOC_HIDE
    filter  = awful.widget.taglist.filter.all, --DOC_HIDE
    buttons = taglist_buttons --DOC_HIDE
} --DOC_HIDE

s.mytaglist.forced_width = 100 --DOC_HIDE
s.mytaglist.forced_height = 18 --DOC_HIDE
s.mytaglist._do_taglist_update_now() --DOC_HIDE

parent:add(wibox.widget.background(s.mytaglist, beautiful.bg_normal)) --DOC_HIDE
