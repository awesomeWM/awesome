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

runner.run_steps({
    function()
        assert(wb.shape == shape.powerline)
        assert(wb.shape_bounding) -- This is a memory leak! Don't copy!
        if was_drawn then
            return true
        end
    end
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
