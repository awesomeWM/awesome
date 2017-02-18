-- Test the selection API

local runner = require("_runner")
local awful = require("awful")

local clear_clipboard = [[
local lgi = require("lgi")
local Gdk = lgi.Gdk
local Gtk = lgi.Gtk
local GLib = lgi.GLib
local clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
clipboard:set_text("This is an experiment", -1)
GLib.idle_add(GLib.PRIORITY_DEFAULT, Gtk.main_quit)
Gtk.main()
]]
local set_clipboard = [[
local Gdk = require("lgi").Gdk
local Gtk = require("lgi").Gtk
local clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
clipboard:set_text("This is an experiment", -1)
print("clipboard set")
Gtk.main()
]]

local success
local continue
local test_routine = coroutine.create(function()
    -- Clear the clipboard
    awful.spawn.with_line_callback({ "lua", "-e", clear_clipboard },
        { exit = function() continue = true end })
    coroutine.yield()

    -- Now check how an unowned clipboard behaves
    local clip = selection("CLIPBOARD")
    clip:request("TARGETS", function(...)
        assert(select('#', ...) == 0)
        continue = true
    end)
    coroutine.yield()

    clip:request("UTF8_STRING", function(...)
        assert(select('#', ...) == 0)
        continue = true
    end)
    coroutine.yield()

    -- Set the clipboard
    awful.spawn.with_line_callback({ "lua", "-e", set_clipboard },
        { stdout = function() continue = true end })
    coroutine.yield()

    -- Now query the clipboard
    local clip = selection("CLIPBOARD")
    clip:request("TARGETS", function(...)
        local targets = { ... }
        assert(awful.util.table.hasitem(targets, "UTF8_STRING"),
            require("gears.debug").dump_return(targets))
        continue = true
    end)
    coroutine.yield()

    clip:request("UTF8_STRING", function(data)
        assert(data == "This is an experiment", data)
        continue = true
    end)
    coroutine.yield()

    success = true
end)
assert(coroutine.resume(test_routine))

runner.run_steps({
    function()
        if continue then
            continue = false
            assert(coroutine.resume(test_routine))
        end
        if coroutine.status(test_routine) == "dead" then
            return success
        end
    end,
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
