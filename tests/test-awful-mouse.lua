require("awful.mouse")

local steps = {}

-- warning: order is important
local buttons = {"left", "middle", "right"}

-- Check the is_****_mouse_button_pressed properties.
for k, v in ipairs(buttons) do

    -- Press the button.
    table.insert(steps, function()
        root.fake_input("button_press", k)
        return true
    end)

    -- Check the property matrix.
    table.insert(steps, function()
        if not mouse["is_"..v.."_mouse_button_pressed"] then return end

        for _, v2 in ipairs(buttons) do
            assert(mouse["is_"..v2.."_mouse_button_pressed"] == (v == v2))
        end

        root.fake_input("button_release", k)

        return true
    end)

    -- Release the button.
    table.insert(steps, function()
        if mouse["is_"..v.."_mouse_button_pressed"] then return end

        return true
    end)

end

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
