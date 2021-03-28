--DOC_GEN_IMAGE --DOC_NO_USAGE
local parent = ... --DOC_HIDE
local gears = { --DOC_HIDE
    watcher  = require("gears.watcher"), --DOC_HIDE
    reactive = require("gears.reactive"), --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

local called = {} --DOC_HIDE

function gears.watcher._read_async(path, callback, _) --DOC_HIDE
    if path == '/sys/class/power_supply/BAT0/energy_full' then --DOC_HIDE
        callback('/sys/class/power_supply/BAT0/energy_full', "89470000") --DOC_HIDE
    elseif path == '/sys/class/power_supply/BAT0/energy_now' then --DOC_HIDE
        callback('/sys/class/power_supply/BAT0/energy_now', "85090000") --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE


    local battery = gears.watcher {
        labels_as_properties = true,
        interval             = 5,
        files                = {
            capacity = "/sys/class/power_supply/BAT0/energy_full",
            current  = "/sys/class/power_supply/BAT0/energy_now"
        },
        filter               = function(label, content)
            called[label] = true --DOC_HIDE
            return tonumber(content)
        end,
    }

    --DOC_NEWLINE

    -- Use the above in a widget.
    local w = wibox.widget {
        text = gears.reactive(function()
            return (battery.current / battery.capacity) .. "%"
        end),
        widget = wibox.widget.textbox
    }

    require("gears.timer").run_delayed_calls_now() --DOC_HIDE
    require("gears.timer").run_delayed_calls_now() --DOC_HIDE
    require("gears.timer").run_delayed_calls_now() --DOC_HIDE

    assert(called["capacity"]) --DOC_HIDE
    assert(called["current"]) --DOC_HIDE
    assert(battery) --DOC_HIDE

parent:add(w) --DOC_HIDE
