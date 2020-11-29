--DOC_GEN_IMAGE
local gears = { watcher = require("gears.watcher") } --DOC_HIDE

    local mybatterydischarging = gears.watcher {
        file     = "/sys/class/power_supply/BAT0/status",
        interval = 5,
    }

    assert(mybatterydischarging) --DOC_HIDE
