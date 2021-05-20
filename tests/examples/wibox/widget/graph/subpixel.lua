--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

local data = { --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
} --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    forced_height = 100, --DOC_HIDE
    forced_width  = 100, --DOC_HIDE
    spacing       = 0, --DOC_HIDE
    layout        = wibox.layout.flex.vertical --DOC_HIDE
} --DOC_HIDE

for _, small_step in ipairs {1, 1/2, 1/3} do
    _ = small_step --DOC_HIDE silence luacheck unused var warning
end --DOC_HIDE actually use truncated numbers, hopefully helps with reproducibility

for _, small_step in ipairs {1, 0.5, 21845/65536} do --DOC_HIDE
    local w = --DOC_HIDE
    wibox.widget {
        -- width = 1000,
        max_value    = 29,
        step_width   = small_step,
        step_spacing = 0,
        color        = "#ff0000",
        background_color = "#000000",
        border_width = 2, --DOC_HIDE
        margins      = 0, --DOC_HIDE
        widget       = wibox.widget.graph,
    }

    l:add(w) --DOC_HIDE

    for _ = 1, 16 do --DOC_HIDE repeat the data enough times to fill the graphs
        for _, v in ipairs(data) do --DOC_HIDE
            w:add_value(v) --DOC_HIDE
        end --DOC_HIDE
    end --DOC_HIDE
end

parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
