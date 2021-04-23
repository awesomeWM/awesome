--DOC_GEN_IMAGE --DOC_HIDE_ALL
local parent = ... --DOC_NO_USAGE
local awful = {
    tag = require("awful.tag"),
    layout = require("awful.layout"),
    placement = require("awful.placement"),
    widget = {taglist = require("awful.widget.taglist")}
}
local wibox = require("wibox")
local beautiful = require("beautiful")

local s = screen[1]
local taglist_buttons = nil -- To make luacheck shut up

local tags = {}

table.insert(tags,
    awful.tag.add("one", {})
)

--DOC_NEWLINE

table.insert(tags,
    awful.tag.add("two", {
        icon = beautiful.awesome_icon
    })
)

--DOC_NEWLINE

table.insert(tags,
    awful.tag.add("three", {})
)

--DOC_NEWLINE

for i=1, 3 do tags[i].selected = false end
tags[2].selected = true

--DOC_HIDE add some clients to some tags
local c = client.gen_fake {x = 80, y = 55, width=75, height=50}
local c2 = client.gen_fake {x = 80, y = 55, width=75, height=50}
c:tags(tags[3])
c2:tags(tags[1])

s.mytaglist = awful.widget.taglist {
    screen  = s,
    filter  = awful.widget.taglist.filter.all,
    buttons = taglist_buttons
}

s.mytaglist.forced_width = 100
s.mytaglist.forced_height = 18
s.mytaglist._do_taglist_update_now()

parent:add(wibox.widget.background(s.mytaglist, beautiful.bg_normal))
