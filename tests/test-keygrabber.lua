-- Test for #2712: It should be possible to create multiple instances of
-- keygrabber during the same loop.

local runner = require("_runner")
local awful = require("awful")

local keygrabber_a_active = false
local keygrabber_b_active = false
local keygrabber_c_active = false

-- Disable the deprecation to test both the current and legacy APIs.
local gdebug = require("gears.debug")
gdebug.deprecate = function() end

local steps = {
    function()

        awful.keygrabber {
            keybindings = {
                awful.key {
                    modifiers = {},
                    key       = "a",
                    on_press  = function() keygrabber_a_active = true end
                }
            },
            stop_key = "Escape",
            stop_callback = function() keygrabber_a_active = false end,
            export_keybindings = true,
        }
        awful.keygrabber {
            keybindings = {
                {{}, "b", function() keygrabber_b_active = true end},
            },
            stop_key = "Escape",
            stop_callback = function() keygrabber_b_active = false end,
            export_keybindings = true,
        }

        local kgc = awful.keygrabber {
            keybindings = {},
            stop_key = "Escape",
            stop_callback = function() keygrabber_c_active = false end,
            export_keybindings = true,
        }
        kgc:add_keybinding(
            awful.key {
                modifiers = {},
                key       = "c",
                on_press  = function() keygrabber_c_active = true end
            }
        )

        return true
    end,

    function(count)
        if count == 1 then
            root.fake_input("key_press"  , "a")
            root.fake_input("key_release", "a")
        end
        if keygrabber_a_active and not keygrabber_b_active then
            return true
        end
    end,

    function(count)
        if count == 1 then
            root.fake_input("key_press"  , "Escape")
            root.fake_input("key_release", "Escape")
        end
        if not keygrabber_a_active and not keygrabber_b_active then
            return true
        end
    end,

    function(count)
        if count == 1 then
            root.fake_input("key_press"  , "b")
            root.fake_input("key_release", "b")
        end
        if not keygrabber_a_active and keygrabber_b_active then
            return true
        end
    end,

    function(count)
        if count == 1 then
            root.fake_input("key_press"  , "Escape")
            root.fake_input("key_release", "Escape")
        end
        if not keygrabber_a_active and not keygrabber_b_active then
            return true
        end
    end,

    function(count)
        if count == 1 then
            root.fake_input("key_press"  , "c")
            root.fake_input("key_release", "c")
        end
        if not keygrabber_a_active and not keygrabber_b_active and keygrabber_c_active then
            return true
        end
    end,

    function(count)
        if count == 1 then
            root.fake_input("key_press"  , "Escape")
            root.fake_input("key_release", "Escape")
        end
        if not keygrabber_a_active and not keygrabber_b_active and not keygrabber_c_active then
            return true
        end
    end
}
runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
