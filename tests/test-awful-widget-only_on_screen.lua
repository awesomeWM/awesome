-- Test that awful.widget.only_on_screen works correctly

local runner = require("_runner")
local wibox = require("wibox")
local awful = require("awful")

-- Make sure we have at least two screens to test this on.
local origin_width = screen[1].geometry.width
screen[1]:fake_resize(
    screen[1].geometry.x,
    screen[1].geometry.y,
    origin_width/2,
    screen[1].geometry.height
)

screen.fake_add(
    screen[1].geometry.x+origin_width/2,
    screen[1].geometry.y,
    origin_width/2,
    screen[1].geometry.height
)

assert(screen.count() == 2)

-- Each screen gets a wibox displaying our only_on_screen widget
local inner_widget = wibox.widget.textbox("This is only visible on some screen")
local only_widget = awful.widget.only_on_screen()
awful.screen.connect_for_each_screen(function(s)
    local geo = s.geometry
    -- Just so that :fit() also gets code coverage, we use an align layout
    local widget = wibox.layout.align.horizontal(only_widget)
    s.testwibox = wibox{
        x = geo.x + geo.width / 4,
        y = geo.y + geo.height / 4,
        width = geo.width / 2,
        height = geo.height / 2,
        widget = widget,
        visible = true,
    }
end)

-- Check if the inner_widget is visible on the given screen
local function widget_visible_on(s)
    for _, entry in ipairs(s.testwibox:find_widgets(1, 1)) do
        if entry.widget == inner_widget then
            return true
        end
    end
    return false
end

-- Test the widget's constructor
do
    local widget = awful.widget.only_on_screen()
    assert(widget.screen == "primary")
    assert(widget.widget == nil)

    widget = awful.widget.only_on_screen(inner_widget, 42)
    assert(widget.screen == 42)
    assert(widget.widget == inner_widget)
end

local steps = {}

-- Test that the widget "does nothing" when no widget is set
table.insert(steps, function()
    -- Idle step so that the testwibox added above is drawn
    return true
end)
table.insert(steps, function()
    for s in screen do
        local res = s.testwibox:find_widgets(1, 1)
        assert(#res == 1, #res)
    end
    return true
end)

-- Test that the widget by default is displayed on the primary screen
table.insert(steps, function()
    only_widget.widget = inner_widget
    return true
end)
table.insert(steps, function()
    assert(only_widget.screen == "primary", only_widget.screen)
    for s in screen do
        if s == screen.primary then
            assert(widget_visible_on(s))
        else
            assert(not widget_visible_on(s))
        end
    end
    return true
end)

-- Test setting the screen
for s in screen do
    local index = s.index
    local function check_result()
        for t in screen do
            assert(widget_visible_on(t) == (t.index == index))
        end
        return true
    end

    -- Test by screen object
    table.insert(steps, function()
        only_widget.screen = s
        return true
    end)
    table.insert(steps, check_result)

    -- Test by index
    table.insert(steps, function()
        only_widget.screen = index
        return true
    end)
    table.insert(steps, check_result)
end

-- Test that the widget is correctly updated when screen "2" is removed
table.insert(steps, function()
    only_widget.screen = 2
    return true
end)
table.insert(steps, function()
    screen[2]:fake_remove()
    return true
end)
table.insert(steps, function()
    for s in screen do
        assert(not widget_visible_on(s))
    end
    screen.fake_add(
        screen[1].geometry.x+origin_width/2,
        screen[1].geometry.y,
        origin_width/2,
        screen[1].geometry.height
    )
    return true
end)
table.insert(steps, function()
    for s in screen do
        assert(widget_visible_on(s) == (s.index == 2))
    end
    return true
end)

-- Test what happens when the primary screen is removed
table.insert(steps, function()
    only_widget.screen = "primary"
    return true
end)
table.insert(steps, function()
    local s, s2 = screen.primary, screen[2]
    assert(s.index == 1)
    s:fake_remove()
    assert(screen.primary == s2)
    return true
end)
table.insert(steps, function()
    assert(only_widget.screen == "primary", only_widget.screen)
    for s in screen do
        if s == screen.primary then
            assert(widget_visible_on(s))
        else
            assert(not widget_visible_on(s))
        end
    end
    return true
end)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
