--DOC_NO_USAGE

local awful = require("awful") --DOC_HIDE

    awful.key({ "Mod4", "Shift" }, "a",
        function () print("The `Mod4` + `Shift` + `a` combo is pressed") end,
        function () print("The `Mod4` + `Shift` + `a` combo is released") end)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
