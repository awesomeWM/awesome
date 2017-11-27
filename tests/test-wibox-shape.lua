-- A quick-and-dirty test of wibox shapes ("Does it error out?")

local runner = require("_runner")
local wibox = require("wibox")
local shape = require("gears.shape")

local was_drawn
local widget = wibox.widget.base.make_widget()
function widget.draw()
    was_drawn = true
end

local wb = wibox {
    shape = shape.powerline,
    widget = widget,
    border_width = 42,
}
wb:geometry(screen[1].geometry)
wb.visible = true

local count = 0

local steps = {
    function()
        assert(wb.shape == shape.powerline)
        assert(wb.shape_bounding) -- This is a memory leak! Don't copy!
        if was_drawn then
            return true
        end
    end,
    -- Remove the shape and test the input
    function()
        wb.shape = nil
        wb.input_passthrough = false
        wb:connect_signal("button::press", function()
            count = count + 1
        end)

        wb:geometry {
            x      = 0,
            y      = 100,
            width  = 101,
            height = 101,
        }
        wb.border_width = 0

        return true
    end
}

-- Emulate a click.
-- Each pair of the `...` corresponds to a point.
local function click(...)
    local args = {...}
    table.insert(steps, function()
        for i = 0, math.floor(#args/2)-1 do
            local x, y = args[i*2+1], args[i*2+2]
            mouse.coords{x=x, y=y}
            assert(mouse.coords().x == x and mouse.coords().y == y)
            root.fake_input("button_release", 1)
            root.fake_input("button_press", 1)
        end

        awesome.sync()

        return true
    end)
end

local function check_count(cnt)
    table.insert(steps, function()
        return cnt == count
    end)
end

-- Check each corner
click(0 , 100,
      99, 100,
      99, 199,
      0 , 199
)

check_count(4)

table.insert(steps, function()
    wb.input_passthrough = true
    count = 0
    return true
end)

-- Do it again
click(0 , 100,
      99, 100,
      99, 199,
      0 , 199
)

-- It's passthrough, so there should be no recorded clicks.
check_count(0)

table.insert(steps, function()
    wb.input_passthrough = false
    count = 0
    return true
end)

table.insert(steps, function()
    awesome.sync()
    count = 0
    return true
end)

-- One last time
click(0 , 100,
      99, 100,
      99, 199,
      0 , 199
)
check_count(4)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
