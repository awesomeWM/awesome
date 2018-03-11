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
local set_clipboard_text = [[
local lgi = require("lgi")
local Gdk = lgi.Gdk
local Gtk = lgi.Gtk
local clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
clipboard:set_text("This is an experiment", -1)
print("clipboard set")
Gtk.main()
]]
local set_clipboard_pixbuf = [[
local lgi = require("lgi")
local Gdk = lgi.Gdk
local Gtk = lgi.Gtk
local GdkPixbuf = lgi.GdkPixbuf
local buf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, true, 8, 1900, 1600)
local clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
clipboard:set_image(buf)
print("clipboard set")
Gtk.main()
]]

local success
local continue

local function request_and_collect(selection, name)
    local result = {}
    selection:request(name, function(...)
        local n = select('#', ...)
        for i = 1, n do
            table.insert(result, (select(i, ...)))
        end
        if n == 0 then
            continue = true
        end
    end)
    coroutine.yield()
    return table.pack(result)
end

local function assert_empty_result(selection, name)
    local res = request_and_collect(selection, "TARGETS")
    if #res ~= 1 or #res[1] ~= 0 then
        error("Expected nothing, but found: " .. require("gears.debug").dump_return(res))
    end
end

local test_routine = coroutine.create(function()
    -- Clear the clipboard
    awful.spawn.with_line_callback({ "lua", "-e", clear_clipboard },
        { exit = function() continue = true end })
    coroutine.yield()

    -- Now check how an unowned clipboard behaves
    local clip = selection("CLIPBOARD")
    assert_empty_result(clip, "TARGETS")
    assert_empty_result(clip, "UTF8_STRING")

    local saw_change = false
    clip:connect_signal("selection_changed", function()
        assert(not saw_change)
        saw_change = true
    end)
    awesome.sync()

    -- Set the clipboard with some short test
    awful.spawn.with_line_callback({ "lua", "-e", set_clipboard_text },
        { stdout = function() continue = true end })
    coroutine.yield()

    assert(saw_change)

    -- Now query the clipboard
    local clip = selection("CLIPBOARD")
    local targets = request_and_collect(clip, "TARGETS")
    assert(awful.util.table.hasitem(targets[1], "UTF8_STRING"),
        require("gears.debug").dump_return(targets))

    local data = request_and_collect(clip, "UTF8_STRING")
    assert(data[1][1] == "This is an experiment", require("gears.debug").dump_return(data))

    -- Set the clipboard with a big picture
    saw_change = false
    awful.spawn.with_line_callback({ "lua", "-e", set_clipboard_pixbuf },
        { stdout = function() continue = true end })
    coroutine.yield()

    assert(saw_change)

    -- Now query the clipboard
    local clip = selection("CLIPBOARD")
    local targets = request_and_collect(clip, "TARGETS")
    assert(awful.util.table.hasitem(targets[1], "image/bmp"),
        require("gears.debug").dump_return(targets))

    local data = request_and_collect(clip, "image/bmp")
    -- Have to massage this data slightly to get it into form, sigh
    local results = {}
    for _, v in ipairs(data) do
        table.insert(results, v[1])
    end
    local string = table.concat(results)

    -- Now check if the string really is what we expected
    local expected = string.char(0x42,
        0x4d, 0x36, 0x29, 0x8b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00,
        0x00, 0x28, 0x00, 0x00, 0x00, 0x6c, 0x07, 0x00, 0x00, 0x40, 0x06, 0x00,
        0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x8b)
    expected = expected .. string.rep(string.char(0), 262144 - #expected)
    assert(string == expected)

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
