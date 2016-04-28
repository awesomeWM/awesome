-- Some memory leak checks as integration tests.

local runner = require("_runner")
local awful = require("awful")
local cairo = require("lgi").cairo
local create_wibox = require("_wibox_helper").create_wibox
local wibox = require("wibox")

local prepare_for_collect = nil
local function emit_refresh()
    awesome.emit_signal("refresh")
end

-- Make the layoutbox in the default config GC'able
-- luacheck: globals mywibox mylayoutbox
for s in screen do
    mywibox[s].visible = false
end
mywibox = nil
mylayoutbox = nil
emit_refresh()

-- Test if some objects can be garbage collected
local function collectable(a, b, c, d, e, f, g, h, last)
    assert(last == nil, "got more arguments than supported")
    local objs = setmetatable({ a, b, c, d, e, f, g, h }, { __mode = "v" })
    a, b, c, d, e, f, g, h = nil, nil, nil, nil, nil, nil, nil, nil -- luacheck: ignore
    if prepare_for_collect then
        prepare_for_collect()
        prepare_for_collect = nil
    end
    collectgarbage("collect")
    collectgarbage("collect")
    -- Check if the table is now empty
    for _, v in pairs(objs) do
        print("Some object was not garbage collected!")
        error(v)
    end
end

-- Use the layoutbox for testing delayed tooltips
local function tooltip_delayed()
    local l = awful.widget.layoutbox(1)
    local t = l._layoutbox_tooltip
    assert(t)
    return l, t
end

local function tooltip_now()
    local w = wibox.widget.textbox("some textbox")
    local t = awful.tooltip({ objects = {w} })
    return w, t
end

-- First test some basic widgets
collectable(wibox.widget.base.make_widget())
collectable(wibox.widget.textbox("foo"))
collectable(wibox.layout.fixed.horizontal())
collectable(wibox.layout.align.horizontal())

-- Then some random widgets from awful
collectable(awful.widget.launcher({ image = cairo.ImageSurface(cairo.Format.ARGB32, 20, 20), command = "bash" }))
collectable(awful.widget.prompt())
collectable(awful.widget.textclock())

-- Test the layout box. As its widgets are a singleton (per screen), clear them
-- first.
awful.widget.layoutbox.clear()
collectgarbage("collect")
collectgarbage("collect")
collectable(awful.widget.layoutbox(1))

-- Some widgets do things via timer.delayed_call
prepare_for_collect = emit_refresh
collectable(tooltip_delayed())

prepare_for_collect = emit_refresh
collectable(tooltip_now())

prepare_for_collect = emit_refresh
collectable(awful.widget.taglist(1, awful.widget.taglist.filter.all))

prepare_for_collect = emit_refresh
collectable(awful.widget.tasklist(1, awful.widget.tasklist.filter.currenttags))

prepare_for_collect = emit_refresh
collectable(create_wibox())

runner.run_steps({ function() return true end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
