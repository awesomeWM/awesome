local awful = require("awful")

-- Widget and layout library
local wibox = require("wibox")

screen.connect_signal("request::desktop_decoration", function(s)




    -- Create the wibox
    s.mywibox = awful.wibar({ position = "top", screen = s })

    local left = {
            layout = wibox.layout.fixed.horizontal,
    }
    local middle = {
            layout = wibox.layout.fixed.horizontal,
    }
    local right = {
            layout = wibox.layout.fixed.horizontal,
    }
    for _,item in ipairs(globalwidgets.left) do
        if item.perscreen then
            table.insert(left, item(s))
        else
            table.insert(left, item)
        end
    end
    for _,item in ipairs(globalwidgets.middle) do
        if item.perscreen then
            table.insert(middle, item(s))
        else
            table.insert(middle, item)
        end
    end
    for _,item in ipairs(globalwidgets.right) do
        if item.perscreen then
            table.insert(right, item(s))
        else
            table.insert(right, item)
        end
    end
    -- Add widgets to the wibox
    s.mywibox.widget = {
        layout = wibox.layout.align.horizontal,
        left,
        middle,
        right
    }
end)
