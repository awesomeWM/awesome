-- Test the selection ownership and transfer API

local runner = require("_runner")
local spawn = require("awful.spawn")

local lua_executable = os.getenv("LUA_EXECUTABLE")
if lua_executable == nil or lua_executable == "" then
    lua_executable = "lua"
end

-- Assemble data for the large transfer that will be done later
local large_transfer_piece = "a"
for _ = 1, 25 do
    large_transfer_piece = large_transfer_piece .. large_transfer_piece
end
large_transfer_piece = large_transfer_piece .. large_transfer_piece .. large_transfer_piece

local large_transfer_piece_count = 3
local large_transfer_size = #large_transfer_piece * large_transfer_piece_count

local header = [[
local lgi = require("lgi")
local Gdk = lgi.Gdk
local Gtk = lgi.Gtk
local GLib = lgi.GLib
local function assert_equal(a, b)
    assert(a == b,
        string.format("%s == %s", a or "nil/false", b or "nil/false"))
end
local clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
]]

local acquire_and_clear_clipboard = header .. [[
clipboard:set_text("This is an experiment", -1)
GLib.idle_add(GLib.PRIORITY_DEFAULT, Gtk.main_quit)
Gtk.main()
]]

local done_footer = [[
io.stdout:write("done")
io.stdout:flush()
]]

local check_targets_and_text = header .. [[
local targets = clipboard:wait_for_targets()
assert_equal(targets[1]:name(), "TARGETS")
assert_equal(targets[2]:name(), "UTF8_STRING")
assert_equal(#targets, 2)
assert_equal(clipboard:wait_for_text(), "Hello World!")
]] .. done_footer

local check_large_transfer = header
    .. string.format("\nassert_equal(#clipboard:wait_for_text(), %d)\n", large_transfer_size)
    .. done_footer

local check_empty_selection = header .. [[
assert_equal(clipboard:wait_for_targets(), nil)
assert_equal(clipboard:wait_for_text(), nil)
]] .. done_footer

local selection_object
local selection_released
local continue

runner.run_steps({
    function()
        -- Get the selection
        local s = assert(selection.acquire{ selection = "CLIPBOARD" },
            "Failed to acquire the clipboard selection")

        -- Steal selection ownership from ourselves and test that it works
        local s_released
        s:connect_signal("release", function() s_released = true end)

        selection_object = assert(selection.acquire{ selection = "CLIPBOARD" },
            "Failed to acquire the clipboard selection")

        assert(s_released)

        -- Now test selection transfers
        selection_object = assert(selection.acquire{ selection = "CLIPBOARD" },
            "Failed to acquire the clipboard selection")
        selection_object:connect_signal("request", function(_, target, transfer)
            if target == "TARGETS" then
                transfer:send{
                    format = "atom",
                    data = { "TARGETS", "UTF8_STRING" },
                }
            elseif target == "UTF8_STRING" then
                transfer:send{ data = "Hello World!" }
            end
        end)
        awesome.sync()
        spawn.with_line_callback({ lua_executable, "-e", check_targets_and_text },
            { stdout = function(line)
                assert(line == "done", "Unexpected line: " .. line)
                continue = true
            end })
        return true
    end,

    function()
        -- Wait for the previous test to succeed
        if not continue then return end
        continue = false

        -- Now test piece-wise selection transfers
        selection_object = assert(selection.acquire{ selection = "CLIPBOARD" },
            "Failed to acquire the clipboard selection")
        selection_object:connect_signal("request", function(_, target, transfer)
            if target == "TARGETS" then
                transfer:send{
                    format = "atom",
                    data = { "TARGETS", "UTF8_STRING" },
                }
            elseif target == "UTF8_STRING" then
                local offset = 1
                local data = "Hello World!"
                local function send_piece()
                    local piece = data:sub(offset, offset)
                    transfer:send{
                        data = piece,
                        continue = piece ~= ""
                    }
                    offset = offset + 1
                end
                transfer:connect_signal("continue", send_piece)
                send_piece()
            end
        end)
        awesome.sync()
        spawn.with_line_callback({ lua_executable, "-e", check_targets_and_text },
            { stdout = function(line)
                assert(line == "done", "Unexpected line: " .. line)
                continue = true
            end })
        return true
    end,

    function()
        -- Wait for the previous test to succeed
        if not continue then return end
        continue = false

        -- Now test a huge transfer
        selection_object = assert(selection.acquire{ selection = "CLIPBOARD" },
            "Failed to acquire the clipboard selection")
        selection_object:connect_signal("request", function(_, target, transfer)
            if target == "TARGETS" then
                transfer:send{
                    format = "atom",
                    data = { "TARGETS", "UTF8_STRING" },
                }
            elseif target == "UTF8_STRING" then
                local count = 0
                local function send_piece()
                    count = count + 1
                    local done = count == large_transfer_piece_count
                    transfer:send{
                        data = large_transfer_piece,
                        continue = not done,
                    }
                end
                transfer:connect_signal("continue", send_piece)
                send_piece()
            end
        end)
        awesome.sync()
        spawn.with_line_callback({ lua_executable, "-e", check_large_transfer },
            { stdout = function(line)
                assert(line == "done", "Unexpected line: " .. line)
                continue = true
            end })
        return true
    end,

    function()
        -- Wait for the previous test to succeed
        if not continue then return end
        continue = false

        -- Now test that :release() works
        selection_object:release()
        awesome.sync()
        spawn.with_line_callback({ lua_executable, "-e", check_empty_selection },
            { stdout = function(line)
                assert(line == "done", "Unexpected line: " .. line)
                continue = true
            end })

        return true
    end,

    function()
        -- Wait for the previous test to succeed
        if not continue then return end
        continue = false

        -- Test for "release" signal when we lose selection
        selection_object = assert(selection.acquire{ selection = "CLIPBOARD" },
            "Failed to acquire the clipboard selection")
        selection_object:connect_signal("release", function() selection_released = true end)
        awesome.sync()
        spawn.with_line_callback({ lua_executable, "-e", acquire_and_clear_clipboard },
            { exit = function() continue = true end })
        return true
    end,

    function()
        -- Wait for the previous test to succeed
        if not continue then return end
        continue = false
        assert(selection_released)
        return true
    end,
}, {
    -- Use a larger step timeout for the large data transfer, which
    -- transfers 3 * 2^25 bytes of data (96 MiB), and takes a while.
    wait_per_step = 10,
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
