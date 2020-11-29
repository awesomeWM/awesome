--DOC_NO_USAGE --DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local gears = require("gears") --DOC_HIDE

local data = { --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
} --DOC_HIDE

local my_source_object = gears.object { --DOC_HIDE
    enable_properties   = true, --DOC_HIDE
    enable_auto_signals = true  --DOC_HIDE
}--DOC_HIDE

my_source_object.value = 0 --DOC_HIDE

-- luacheck: globals my_graph my_label my_progress --DOC_HIDE

local w = --DOC_HIDE
    wibox.widget {
        {
            {
                id        = "my_graph",
                max_value = 30,
                widget    = wibox.widget.graph
            },
            {
                id     = "my_label",
                align  = "center",
                valign = "center",
                widget = wibox.widget.textbox,
            },
            layout = wibox.layout.stack
        },
        id            = "my_progress",
        max_value     = 30,
        min_value     = 0,
        forced_height = 30, --DOC_HIDE
        forced_width  = 200, --DOC_HIDE
        widget        = wibox.container.radialprogressbar,

        --DOC_NEWLINE
        -- Set the value of all 3 widgets.
        gears.connection {
            source          = my_source_object,
            source_property = "value",
            callback        = function(_, _, value)
                my_graph:add_value(value)
                my_label.text = value .. "mB/s"
                my_progress.value = value
            end
        },
    }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE

for _, v in ipairs(data) do --DOC_HIDE
    assert(v ~= nil) --DOC_HIDE
    my_source_object.value = v --DOC_HIDE
end --DOC_HIDE

parent:add(w) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
