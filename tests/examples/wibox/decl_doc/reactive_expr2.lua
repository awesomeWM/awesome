--DOC_GEN_IMAGE --DOC_HIDE --DOC_NO_USAGE --DOC_NO_DASH
local parent = ... --DOC_HIDE
local gears = { --DOC_HIDE
    object   = require("gears.object"), --DOC_HIDE
    reactive = require("gears.reactive") --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

local my_hub = gears.object {--DOC_HIDE
    enable_properties   = true,--DOC_HIDE
    enable_auto_signals = true--DOC_HIDE
} --DOC_HIDE

my_hub.some_property = 42 --DOC_HIDE

    -- For some larger function, it's a good idea to move them out of
    -- the declarative construct for maintainability.
    local my_reactive_object = gears.reactive(function()
        if my_hub.some_property > 1000 then
            return "<span fgcolor='#ffff00'>The world is fine</span>"
        else
            return "<span fgcolor='#ff0000'>The world is on fire</span>"
        end
    end)

    --DOC_NEWLINE

    local w = wibox.widget {
        markup = my_reactive_object,
        forced_height = 20, --DOC_HIDE
        widget = wibox.widget.textbox
    }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(w) --DOC_HIDE
assert(w.markup == "<span fgcolor='#ff0000'>The world is on fire</span>") --DOC_HIDE

parent:add(w) --DOC_HIDE
