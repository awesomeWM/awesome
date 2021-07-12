--DOC_NO_USAGE

local awful = require("awful") --DOC_HIDE

    awful.key {
        modifiers = { "Mod4", "Shift" },
        key = 'a',
        on_press = function ()
            print("The `Mod4` + `Shift` + `a` combo is pressed")
        end,
        on_release = function ()
            print("The `Mod4` + `Shift` + `a` combo is released")
        end
    }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
