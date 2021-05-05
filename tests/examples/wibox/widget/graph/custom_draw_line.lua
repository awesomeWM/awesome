--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

local data = { --DOC_HIDE
    {5,95,43},{3,96,43},{1,95,46},{0,94,50},{0,91,55}, --DOC_HIDE
    {0,87,62},{1,82,68},{3,76,74},{5,70,79},{8,64,83}, --DOC_HIDE
    {11,57,85},{15,51,85},{20,46,84},{24,41,80},{30,37,75}, --DOC_HIDE
    {35,34,69},{41,32,63},{47,32,57},{53,33,51},{60,35,47}, --DOC_HIDE
    {66,38,44},{73,43,43},{79,48,43},{85,54,46},{91,60,50}, --DOC_HIDE
} --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    forced_height = 100, --DOC_HIDE
    forced_width  = 100, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.vertical --DOC_HIDE
} --DOC_HIDE

local colors = {
    "#ff0000ff",
    "#00ff00ff",
    "#0000ffff"
}

--DOC_NEWLINE

local w = --DOC_HIDE
wibox.widget {
    scale        = true,
    group_colors = colors,
    step_width   = 5,
    step_shape   = function(cr, width)
        local mid = math.floor(width/2) + 0.5
        -- Draw line from the previous data point
        cr:line_to(mid, 0)
        -- Draw a tick
        cr:move_to(mid, -2)
        cr:line_to(mid, 2)
        -- Go back to the center
        cr:move_to(mid, 0)
    end,
    group_finish = function(_, _, cr)
        cr:set_line_width(1)
        cr:stroke()
    end,
    border_width = 2, --DOC_HIDE
    margins      = 5, --DOC_HIDE
    widget       = wibox.widget.graph,
}

l:add(w) --DOC_HIDE

for _, v in ipairs(data) do --DOC_HIDE
    for group, value in ipairs(v) do --DOC_HIDE
        w:add_value(value, group) --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
