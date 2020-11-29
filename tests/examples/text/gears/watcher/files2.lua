--DOC_GEN_IMAGE --DOC_NO_USAGE
local gears = { watcher = require("gears.watcher") } --DOC_HIDE

local called = 0 --DOC_HIDE

function gears.watcher._read_async(path, callback) --DOC_HIDE
    if path == '/sys/class/power_supply/AC/online' then --DOC_HIDE
        called = called + 1 --DOC_HIDE
        callback('/sys/class/power_supply/AC/online', "1") --DOC_HIDE
    elseif path == "/sys/class/power_supply/BAT0/status" then --DOC_HIDE
        called = called + 1 --DOC_HIDE
        callback('/sys/class/power_supply/BAT0/status', "Full") --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

    local mybatterycharging = gears.watcher {
        files    = {
            plugged = "/sys/class/power_supply/AC/online",
            charged = "/sys/class/power_supply/BAT0/status"
        },
        filter   = function(content)
            assert(content.plugged == "1") --DOC_HIDE
            assert(content.charged == "Full") --DOC_HIDE
            return content.plugged == "1" or content.charged == "Full"
        end,
        interval = 5,
    }

    require("gears.timer").run_delayed_calls_now() --DOC_HIDE
    require("gears.timer").run_delayed_calls_now() --DOC_HIDE
    require("gears.timer").run_delayed_calls_now() --DOC_HIDE

    assert(called >= 2) --DOC_HIDE
    assert(mybatterycharging) --DOC_HIDE
