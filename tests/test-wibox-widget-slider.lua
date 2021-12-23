local runner = require("_runner")
local wibox = require("wibox")
local gears = {
    shape = require("gears.shape")
}

local steps = {}

-- The test runner doesn't support it (yet), but it would be nice to have named
-- steps.
local function step(_, func)
    table.insert(steps, func)
end

local w
local slider

local on_mouse_press = nil
local on_drag_start = nil
local on_drag = nil
local on_drag_end = nil
local on_property_value = nil

step("create slider widget", function()
    slider = wibox.widget.slider {
        minimum = 0,
        maximum = 100,
        bar_shape = gears.shape.rounded_rect,
        bar_height = 3,
        bar_color = "#ffffff",
        handle_color = "#0000ff",
        handle_shape = gears.shape.circle,
        handle_border_color = "#ffffff",
        handle_border_width = 1,
    }

    slider:connect_signal("button::press", function(...) on_mouse_press = true end)
    slider:connect_signal("drag_start", function(...) on_drag_start = true end)
    slider:connect_signal("drag", function(...) on_drag = true end)
    slider:connect_signal("drag_end", function(...) on_drag_end = true end)
    slider:connect_signal("property::value", function(...) on_property_value = true end)

    w = wibox {
        ontop = true,
        x = 0,
        y = 0,
        width = 250,
        height = 50,
        visible = true,
        widget = slider,
    }

    return true
end)

step("start dragging", function()
    -- Coordinates to hit the slider's handle
    mouse.coords({ x = 1, y = 24 })

    root.fake_input("button_press", 1)
    awesome.sync()

    return on_drag_start
end)

step("drag handle", function()
    mouse.coords({ x = 50, y = 24 })
    return true
end)

step("stop dragging", function()
    root.fake_input("button_release", 1)
    awesome.sync()

    return on_drag_end
end)

step("check signals", function()
    assert(on_mouse_press, "Signal `button::press` was not called")
    assert(on_drag_start, "Signal `drag_start` was not called")
    assert(on_property_value, "Signal `property::value` was not called")
    assert(on_drag, "Signal `drag` was not called")
    assert(on_drag_end, "Signal `drag_end` was not called")
    return true
end)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
