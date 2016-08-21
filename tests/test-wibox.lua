local runner    = require( "_runner"    )
local wibox     = require( "wibox"      )
local beautiful = require( "beautiful"  )
local shape     = require( "gears.shape")

local steps = {}

local w

-- create a wibox.
table.insert(steps, function()
    w = wibox {
        visible = true,
        ontop   = true,
        x       = 100,
        y       = 100,
        width   = 200,
        height  = 200,
    }

    return true
end)

-- check all properties match
table.insert(steps, function()
    assert(w              )
    assert(w.ontop        )
    assert(w.visible      )
    assert(w.x      == 100)
    assert(w.y      == 100)
    assert(w.width  == 200)
    assert(w.height == 200)

    -- Add a widget
    local tb = wibox.widget {
        text = "Awesome!",
        widget =  wibox.widget.textbox
    }

    w:set_widget(tb)
    assert(tb and w:get_widget() == tb and w.widget == tb)

    w.bg      = "#0000ff"
    w.fg      = "#ff00ff"
    w.bgimage = beautiful.awesome_icon

    return true
end)

-- Test the shape
table.insert(steps, function()

    -- Test the unset code
    w.bg           = nil
    w.bgimage      = nil
    w.border_width = 5
    w.border_color = "#ff0000"
    w.shape = shape.infobubble

    return true
end)

-- Change the border
table.insert(steps, function()
    w.border_width = 1
    w.border_color = "#00ff00"

    return true
end)

-- Remove the border
table.insert(steps, function()
    w.border_width = 0

    return true
end)

-- Check if the position is back to the original
table.insert(steps, function()
    assert(w              )
    assert(w.ontop        )
    assert(w.visible      )
    assert(w.x      == 100)
    assert(w.y      == 100)
    assert(w.width  == 200)
    assert(w.height == 200)

    -- Other tests
    assert(w.screen.index == 1)
    w.screen = 1

    return true
end)


runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
