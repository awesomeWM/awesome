local runner    = require( "_runner"   )
local wibox     = require( "wibox"     )
local awful     = require( "awful"     )
local beautiful = require( "beautiful" )
local gtable    = require("gears.table")

local steps = {}

local w
local img
local button

-- Also check recursive signals from events
local layout
local got_called = false

-- create a wibox.
table.insert(steps, function()

    w = wibox {
        ontop   = true,
        width   = 250,
        height  = 250,
        visible = true,
    }

    button = awful.widget.button {
        image = beautiful.awesome_icon
    }

    w : setup {
        {
            {
                text   = "foo",
                widget = wibox.widget.textbox,
            },
            bg     = "#ff0000",
            widget = wibox.container.background
        },
        {
            {
                widget = button,
            },
            bg     = "#ff00ff",
            widget = wibox.container.background
        },
        {
            {
                text   = "foo",
                widget = wibox.widget.textbox,
            },
            bg     = "#0000ff",
            widget = wibox.container.background
        },
        layout = wibox.layout.flex.vertical
    }

    awful.placement.centered(w)

    img = button._private.image
    assert(img)

    -- Test the click
    layout = w.widget
    assert(layout)

    button:buttons(gtable.join(
        button:buttons(),
        awful.button({}, 1, nil, function ()
            button:emit_signal_recursive("test::recursive")
        end)
))

    layout:connect_signal("test::recursive", function()
        got_called = true
    end)

    return true
end)

-- Test a button press
table.insert(steps, function()
    awful.placement.centered(mouse)

    root.fake_input("button_press", 1)
    awesome.sync()

    return true
end)

table.insert(steps, function()
    assert(button._private.image ~= img)

    return true
end)

-- Test a button release
table.insert(steps, function()
    assert(button._private.image ~= img)

    root.fake_input("button_release", 1)
    awesome.sync()

    return true
end)

-- Test a button press/release outside of the widget
table.insert(steps, function()
    assert(button._private.image == img)

    root.fake_input("button_press", 1)
    awesome.sync()

    return true
end)

table.insert(steps, function()
assert(button._private.image ~= img)

return true
end)

table.insert(steps, function()
-- just make sure the button is not released for nothing
assert(button._private.image ~= img)

-- test if the button is released when the mouse move out
awful.placement.right(mouse--[[, {parent = w}]])
root.fake_input("button_release", 1)
awesome.sync()

return true
end)

table.insert(steps, function()
assert(button._private.image == img)

-- The button had plenty of clicks by now. Make sure everything worked
assert(got_called)

return true
end)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
