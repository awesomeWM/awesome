-- Test the selection watcher API

local runner = require("_runner")
local spawn = require("awful.spawn")

local header = [[
local lgi = require("lgi")
local Gdk = lgi.Gdk
local Gtk = lgi.Gtk
local GLib = lgi.GLib
local clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
clipboard:set_text("This is an experiment", -1)
]]

local acquire_and_clear_clipboard = header .. [[
GLib.idle_add(GLib.PRIORITY_DEFAULT, Gtk.main_quit)
Gtk.main()
]]

local acquire_clipboard = header .. [[
GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 60, Gtk.main_quit)
GLib.idle_add(GLib.PRIORITY_DEFAULT, function()
    print("initialisation done")
    io.stdout:flush()
end)
Gtk.main()
]]

local had_error = false
local owned_clipboard_changes, unowned_clipboard_changes = 0, 0

local clipboard_watcher = selection.watcher("CLIPBOARD")
local clipboard_watcher_inactive = selection.watcher("CLIPBOARD")
local primary_watcher = selection.watcher("PRIMARY")

clipboard_watcher:connect_signal("selection_changed", function(_, owned)
    if owned then
        owned_clipboard_changes = owned_clipboard_changes + 1
    else
        unowned_clipboard_changes = unowned_clipboard_changes + 1
    end
end)
clipboard_watcher_inactive:connect_signal("selection_changed", function()
    had_error = true
    error("Unexpected signal on inactive CLIPBOARD watcher")
end)
primary_watcher:connect_signal("selection_changed", function()
    had_error = true
    error("Unexpected signal on PRIMARY watcher")
end)

local function check_state(owned, unowned)
    assert(not had_error, "there was an error")
    assert(owned_clipboard_changes == owned,
        string.format("expected %d owned changes, but got %d", owned, owned_clipboard_changes))
    assert(unowned_clipboard_changes == unowned,
        string.format("expected %d unowned changes, but got %d", unowned, unowned_clipboard_changes))
end

local continue = false
runner.run_steps{
    -- Clear the clipboard to get to a known state
    function()
        check_state(0, 0)
        spawn.with_line_callback({ os.getenv("LUA_EXECUTABLE"), "-e", acquire_and_clear_clipboard },
            { exit = function() continue = true end })
        return true
    end,

    function()
        -- Wait for the clipboard to be cleared
        check_state(0, 0)
        if not continue then
            return
        end

        -- Activate the watchers
        clipboard_watcher.active = true
        primary_watcher.active = true
        awesome.sync()

        -- Set the clipboard
        continue = false
        spawn.with_line_callback({ os.getenv("LUA_EXECUTABLE"), "-e", acquire_clipboard },
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
        check_state(1, 0)

        -- Now clear the clipboard again
        continue = false
        spawn.with_line_callback({ os.getenv("LUA_EXECUTABLE"), "-e", acquire_and_clear_clipboard },
            { exit = function() continue = true end })

        return true
    end,

    function()
        -- Wait for the clipboard to be set
        if not continue then
            return
        end

        check_state(2, 1)
        return true
    end
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
