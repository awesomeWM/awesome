--DOC_GEN_IMAGE --DOC_HIDE --DOC_NO_USAGE
local parent = ... --DOC_HIDE
local gears = { --DOC_HIDE
    object   = require("gears.object"), --DOC_HIDE
    reactive = require("gears.reactive") --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

    -- It's important to set 'enable_auto_signals' to `true` or it wont work.
    --
    -- Note that most AwesomeWM objects (and most modules) objects can be
    -- used directly as long as they implement the signal `property::` spec.
    --
    -- So you don't *need* a hub object, but it's safer to use one.
    local my_hub = gears.object {
        enable_properties   = true,
        enable_auto_signals = true
    }

    --DOC_NEWLINE

    -- Better set a default value to avoid weirdness.
    my_hub.some_property = 42

    --DOC_NEWLINE

    -- This is an example, in practice do this in your
    -- wibar widget declaration tree.
    local w = wibox.widget {
        markup = gears.reactive(function()
            -- Each time `my_hub.some_property` changes, this will be
            -- re-interpreted.
            return '<i>' .. (my_hub.some_property / 100) .. '</i>'
        end),
        widget = wibox.widget.textbox
    }

    --DOC_NEWLINE

    -- This will update the widget text to '<i>13.37</i>'
    my_hub.some_property = 1337

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(w) --DOC_HIDE
assert(w.markup == "<i>13.37</i>") --DOC_HIDE
parent:add(w) --DOC_HIDE
