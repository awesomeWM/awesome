--DOC_GEN_IMAGE --DOC_HIDE --DOC_NO_USAGE --DOC_NO_DASH
local parent= ... --DOC_HIDE
local gears = require("gears") --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

local obj = nil --DOC_HIDE

gears.timer = function()--DOC_HIDE
    obj = gears.object { --DOC_HIDE
        enable_properties   = true, --DOC_HIDE
        enable_auto_signals = true  --DOC_HIDE
    }--DOC_HIDE
    return obj --DOC_HIDE
end--DOC_HIDE

    local w = wibox.widget {

        gears.connection {
            source   = gears.timer {
                timeout   = 5,
                autostart = true,
            },
            signal   = "timeout",
            callback = function(_, parent_widget)
                parent_widget.text = "this will get called every 5 seconds"
            end
        },

        widget = wibox.widget.textbox
    }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(obj) --DOC_HIDE
obj:emit_signal("timeout") --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(w.text == "this will get called every 5 seconds") --DOC_HIDE
parent:add(w) --DOC_HIDE
