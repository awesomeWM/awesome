-- Test the selection getter API

local runner = require("_runner")
local spawn = require("awful.spawn")
local dump_return = require("gears.debug").dump_return
local gtable = require("gears.table")
local lgi = require("lgi")
local Gio = lgi.Gio
local GdkPixbuf = lgi.GdkPixbuf

local header = [[
local lgi = require("lgi")
local Gdk = lgi.Gdk
local Gtk = lgi.Gtk
local GLib = lgi.GLib
local clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
]]

local set_text = [[
clipboard:set_text("This is an experiment", -1)
]]

local set_pixbuf = [[
local GdkPixbuf = lgi.GdkPixbuf
local buf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, true, 8, 1900, 1600)
clipboard:set_image(buf)
]]

local common_tail = [[
GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 60, Gtk.main_quit)
GLib.idle_add(GLib.PRIORITY_DEFAULT, function()
    print("initialisation done")
    io.stdout:flush()
end)
Gtk.main()
]]

local acquire_and_clear_clipboard = header .. set_text .. [[
GLib.idle_add(GLib.PRIORITY_DEFAULT, Gtk.main_quit)
Gtk.main()
]]

local acquire_clipboard_text = header .. set_text .. common_tail
local acquire_clipboard_pixbuf = header .. set_pixbuf .. common_tail

local continue = false
runner.run_steps{

    -- Clear the clipboard to get to a known state
    function()
        spawn.with_line_callback({ os.getenv("LUA_EXECUTABLE"), "-e", acquire_and_clear_clipboard },
            { exit = function() continue = true end })
        return true
    end,

    function()
        -- Wait for the clipboard to be cleared
        if not continue then
            return
        end

        -- Now query the state of the clipboard (should be empty)
        continue = false
        local s = selection.getter{ selection = "CLIPBOARD", target = "TARGETS" }
        s:connect_signal("data", function(...) error("Got unexpected data: " .. dump_return{...}) end)
        s:connect_signal("data_end", function()
            assert(not continue)
            continue = true
        end)

        return true
    end,

    function()
        -- Wait for the above check to be done
        if not continue then
            return
        end

        -- Now set the clipboard to some text
        continue = false
        spawn.with_line_callback({ os.getenv("LUA_EXECUTABLE"), "-e", acquire_clipboard_text },
            { stdout = function(line)
                assert(line == "initialisation done",
                    "Unexpected line: " .. line)
                continue = true
            end })

        return true
    end,

    function()
        -- Wait for the clipboard to be set
        if not continue then
            return
        end

        -- Query whether the clipboard contains a text (UTF8_STRING)
        continue = false
        local s = selection.getter{ selection = "CLIPBOARD", target = "TARGETS" }
        local data = nil
        s:connect_signal("data", function(_, d)
            assert(not data)
            data = d
        end)
        s:connect_signal("data_end", function()
            assert(gtable.hasitem(data, "UTF8_STRING"))
            assert(not continue)
            continue = true
        end)

        return true
    end,

    function()
        -- Wait for the above check to be done
        if not continue then
            return
        end

        -- Query the text in the clipboard
        continue = false
        local s = selection.getter{ selection = "CLIPBOARD", target = "UTF8_STRING" }
        local data = nil
        s:connect_signal("data", function(_, d)
            assert(data == nil)
            data = d
        end)
        s:connect_signal("data_end", function()
            assert(data == "This is an experiment")
            assert(not continue)
            continue = true
        end)

        return true
    end,

    function()
        -- Wait for the above check to be done
        if not continue then
            return
        end

        -- Now set the clipboard to an image
        continue = false
        spawn.with_line_callback({ os.getenv("LUA_EXECUTABLE"), "-e", acquire_clipboard_pixbuf },
            { stdout = function(line)
                assert(line == "initialisation done",
                    "Unexpected line: " .. line)
                continue = true
            end })

        return true
    end,

    function()
        -- Wait for the above check to be done
        if not continue then
            return
        end

        -- Query the image in the clipboard
        continue = false
        local s = selection.getter{ selection = "CLIPBOARD", target = "image/bmp" }
        local data = {}
        s:connect_signal("data", function(_, d)
            table.insert(data, d)
        end)
        s:connect_signal("data_end", function()
            local image = table.concat(data)
            local stream = Gio.MemoryInputStream.new_from_data(image)
            local pixbuf, err = GdkPixbuf.Pixbuf.new_from_stream(stream)
            assert(not err, tostring(err))
            assert(pixbuf.width == 1900)
            assert(pixbuf.height == 1600)
            assert(pixbuf.colorspace == "RGB")
            assert(pixbuf["has-alpha"] == false)

            assert(not continue)
            continue = true
        end)

        return true
    end,

    function()
        -- Wait for the above check to be done
        if not continue then
            return
        end

        return true
    end,
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
