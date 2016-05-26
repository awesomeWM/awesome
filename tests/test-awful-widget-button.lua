local runner    = require( "_runner"   )
local wibox     = require( "wibox"     )
local awful     = require( "awful"     )
local beautiful = require( "beautiful" )

local steps = {}

local w
local img
local button

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

    return true
end)

-- Test a button press
table.insert(steps, function()
    awful.placement.centered(mouse)

    root.fake_input("button_press", 1)

    return true
end)

table.insert(steps, function()
    assert(button._private.image ~= img)

    return true
end)

-- Test a button release
table.insert(steps, function()
    root.fake_input("button_release", 1)

    assert(button._private.image ~= img)

    return true
end)

-- Test a button press/release outside of the widget
table.insert(steps, function()
    assert(button._private.image == img)

    root.fake_input("button_press", 1)

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

    return true
end)

table.insert(steps, function()
    assert(button._private.image == img)

    return true
end)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
