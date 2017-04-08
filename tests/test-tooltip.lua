local runner = require("_runner")
local place = require("awful.placement")
local wibox = require("wibox")
local beautiful = require("beautiful")
local tooltip = require("awful.tooltip")
local gears = require("gears")

local steps = {}

local w = wibox {
    width   = 250,
    height  = 250,
    visible = true,
    ontop   = true
}

w:setup{
    image  = beautiful.awesome_icon,
    widget = wibox.widget.imagebox
}

-- Center eveything
place.centered(w)
place.centered(mouse)

local tt = nil

table.insert(steps, function()
    tt = tooltip {text = "A long tooltip", visible = true}

    return true
end)

local align_pos = {
    "top_left", "left", "bottom_left", "right",
    "top_right", "bottom_right", "bottom", "top",
}

-- Test the various alignment code paths
for _, v in ipairs(align_pos) do
    table.insert(steps, function()
        tt.align = v

        return true
    end)
end

-- Set a parent object
table.insert(steps, function()
    tt:add_to_object(w)

    return true
end)

-- Test the other mode
table.insert(steps, function()
    tt.mode = "outside"

    -- This only work when there is a mouse::enter event, create one
    place.top_left(mouse)
    place.centered(mouse)

    return true
end)

--- Reset the wibox content and use a widget geometry.
table.insert(steps, function()
    tt:remove_from_object(w)

    tt.visible = false

    w:setup{
        {
            image  = beautiful.awesome_icon,
            widget = wibox.widget.imagebox,
            id     = "myimagebox"
        },
        top    = 125,
        bottom = 100,
        left   = 205,
        right  = 20 ,
        layout = wibox.container.margin
    }

    local imb = w:get_children_by_id("myimagebox")[1]
    assert(imb)

    tt:add_to_object(imb)

    -- Move the mouse over the widget
    place.top_left(mouse)
    mouse.coords {
        x = w.x + w.width - 20 - 12.5,
        y = w.y + 125 + 12.5,
    }
    return true
end)

-- Test that the above move had the intended effect
table.insert(steps, function()
    assert(tt.current_position == "top", tt.current_position)

    return true
end)

-- Try the preferred positions
table.insert(steps, function()
    tt.visible = false

    tt.preferred_positions = {"right"}

    tt.visible = true

    assert(tt.current_position == "right")

    return true
end)

-- Add a shape.
table.insert(steps, function()
    tt.shape = gears.shape.octogon

    return true
end)

-- Set a widget in the tooltip
table.insert(steps, function(count)
    tt:set_widget(wibox.widget.imagebox(beautiful.awesome_icon), 500, 300)

    -- Wait for redraw
    if count > 1 then
        return true
    end
end)

-- And switch back to text
table.insert(steps, function(count)
    tt.text = "I live!"

    -- Wait for redraw
    if count > 1 then
        return true
    end
end)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
