--DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_NO_DASH
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local gears = { watcher = require("gears.watcher") } --DOC_HIDE
local wibox = { widget = require("wibox.widget") }--DOC_HIDE

function gears.watcher._read_async(path, callback, _) --DOC_HIDE
    if path == '/sys/class/power_supply/BAT0/energy_full' then --DOC_HIDE
        callback('/sys/class/power_supply/BAT0/energy_full', "89470000") --DOC_HIDE
    elseif path == '/sys/class/power_supply/BAT0/energy_now' then --DOC_HIDE
        callback('/sys/class/power_supply/BAT0/energy_now', "85090000") --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

    -- In this example, the value of energy_full is 89470000 and the value
    -- of energy_now is 85090000. The result will be 95%.
    local w = --DOC_HIDE
    wibox.widget {
        value = gears.watcher {
            files    = {
                capacity = "/sys/class/power_supply/BAT0/energy_full",
                current  = "/sys/class/power_supply/BAT0/energy_now"
            },
            filter   = function(content)
                return tonumber(content.current) / tonumber(content.capacity)
            end,
            interval = 5,
        },
        forced_width = 100, --DOC_HIDE
        forced_height = 30, --DOC_HIDE
        max_value = 1,
        min_value = 0,
        widget    = wibox.widget.progressbar
    }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(w and w.value >= 0.94 and w.value <= 0.96) --DOC_HIDE

parent:add(w) --DOC_HIDE

