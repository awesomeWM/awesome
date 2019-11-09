--DOC_HEADER --DOC_NO_USAGE

local was_called = {} --DOC_HIDE

local awful = { keygrabber = require("awful.keygrabber"), --DOC_HIDE
    key = require("awful.key"), --DOC_HIDE
    client={focus={history={--DOC_HIDE
        disable_tracking = function() was_called[1] = true end, --DOC_HIDE
        enable_tracking  = function() was_called[2] = true end, --DOC_HIDE
        select_next      = function() was_called[3] = true end, --DOC_HIDE
        select_previous  = function() was_called[4] = true end, --DOC_HIDE
}}}}--DOC_HIDE

    awful.keygrabber {
        keybindings = {
            awful.key {
                modifiers = {"Mod1"},
                key       = "Tab",
                on_press  = awful.client.focus.history.select_previous
            },
            awful.key {
                modifiers = {"Mod1", "Shift"},
                key       = "Tab",
                on_press  = awful.client.focus.history.select_next
            },
        },
        -- Note that it is using the key name and not the modifier name.
        stop_key           = "Mod1",
        stop_event         = "release",
        start_callback     = awful.client.focus.history.disable_tracking,
        stop_callback      = awful.client.focus.history.enable_tracking,
        export_keybindings = true,
    }

--DOC_HIDE Trigger the keybinging
require("gears.timer").run_delayed_calls_now() --DOC_HIDE `export_keybindings` is async
root.fake_input("key_press", "Alt_L")--DOC_HIDE
root.fake_input("key_press", "Tab")--DOC_HIDE
root.fake_input("key_release", "Tab")--DOC_HIDE
root.fake_input("key_release", "Alt_L")--DOC_HIDE
assert(was_called[1] and was_called[1] and was_called[2] and was_called[4])--DOC_HIDE
assert(not was_called[3]) --DOC_HIDE

--DOC_HIDE Now make sure it can be triggered again
root.fake_input("key_press", "Alt_L")--DOC_HIDE
root.fake_input("key_press", "Shift_L")--DOC_HIDE
root.fake_input("key_press", "Tab")--DOC_HIDE
root.fake_input("key_release", "Tab")--DOC_HIDE

assert(was_called[3]) --DOC_HIDE
