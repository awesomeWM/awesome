--DOC_GEN_IMAGE --DOC_NO_USAGE
local gears = { watcher = require("gears.watcher") } --DOC_HIDE

function gears.watcher._read_async(path, callback, _) --DOC_HIDE
    if path == '/sys/class/power_supply/AC/online' then --DOC_HIDE
        callback('/sys/class/power_supply/AC/online', "1") --DOC_HIDE
    elseif path == "/sys/class/power_supply/BAT0/status" then --DOC_HIDE
        callback('/sys/class/power_supply/BAT0/status', "Full") --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

    local mybatterycharging = gears.watcher {
        files    = {
            "/sys/class/power_supply/AC/online",
            "/sys/class/power_supply/BAT0/status"
        },
        filter   = function(content)
            assert(content['/sys/class/power_supply/AC/online'] == "1") --DOC_HIDE
            assert(content['/sys/class/power_supply/BAT0/status'] == "Full") --DOC_HIDE
            return content['/sys/class/power_supply/AC/online'  ] == "1"
                or content['/sys/class/power_supply/BAT0/status'] == "Full"
        end,
        interval = 5,
    }

    assert(mybatterycharging) --DOC_HIDE
