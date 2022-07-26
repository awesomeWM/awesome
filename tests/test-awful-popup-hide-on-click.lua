local runner = require "_runner"
local base = require "wibox.widget.base"
local popup = require "awful.popup"

local widget = base.make_widget()
local p = popup { widget = widget }

local function click(x, y)
    return function()
        mouse.coords { x = x, y = y }
        assert(mouse.coords().x == x and mouse.coords().y == y)
        root.fake_input("button_press", 1)
        awesome.sync()
        root.fake_input("button_release", 1)
        awesome.sync()
        return true
    end
end

runner.run_steps {
    function()
        assert(p.visible)
        return true
    end,
    click(p.x, p.y),
    function()
        assert(p.visible)
        p.hide_on_click = true
        return true
    end,
    click(p.x, p.y),
    function()
        assert(not p.visible)
        return true
    end,
}
