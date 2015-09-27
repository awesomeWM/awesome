-- Some memory leak checks as integration tests.

local awful = require("awful")
local cairo = require("lgi").cairo
local create_wibox = require("_wibox_helper").create_wibox
local gears = require("gears")
local wibox = require("wibox")

local errors = {}

local prepare_for_collect = nil

-- Test if some objects can be garbage collected
local function collectable(a, b, c, d, e, f, g, h)
    local objs = setmetatable({ a, b, c, d, e, f, g, h }, { __mode = "v" })
    a, b, c, d, e, f, g, h = nil, nil, nil, nil, nil, nil, nil, nil
    if prepare_for_collect then
        prepare_for_collect()
        prepare_for_collect = nil
    end
    collectgarbage("collect")
    -- Check if the table is now empty
    for k, v in pairs(objs) do
        print("Some object was not garbage collected!")
        error(v)
    end
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
collectable(awful.widget.layoutbox(1))
collectable(awful.widget.taglist(1, awful.widget.taglist.filter.all))

-- And finally a full wibox
function prepare_for_collect()
    -- Only after doing the pending repaint can a wibox be GC'd.
    awesome.emit_signal("refresh")
end
collectable(create_wibox())

require("_runner").run_steps({ function() return true end })
