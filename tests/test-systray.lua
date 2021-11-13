local spawn = require("awful.spawn")
local wibox = require("wibox")
local beautiful = require("beautiful")

local steps, pid1, pid2, draw_w, st = {}

table.insert(steps, function()
    screen[1].mywibox:remove()

    local widget = wibox.widget.base.make_widget()

    function widget:fit(_, w, h)
        return w, h
    end

    function widget:draw(_, _, width)
        draw_w = width
    end

    local wb = wibox {
        x       = 0,
        y       = 0,
        width   = 100,
        height  = 20,
        visible = true
    }

    st = wibox.widget.systray()

    wb.widget = {
        st,
        widget,
        layout = wibox.layout.fixed.horizontal
    }

    pid1 = spawn("./test-systray")

    return true
end)

table.insert(steps, function()
    if draw_w ~= 80 then return end

    pid2 = spawn("./test-systray")

    return true
end)

table.insert(steps, function()
    if draw_w ~= 60 then return end

    st.reverse = true
    st.horizontal = false

    return true
end)

table.insert(steps, function()
    st.horizontal = true

    beautiful.systray_icon_spacing = 10

    return true
end)


table.insert(steps, function()
    if draw_w ~= 50 then return end

    st.base_size = 5
    return true
end)


table.insert(steps, function()
    if draw_w ~= 80 then return end

    awesome.kill(pid1, 9)
    awesome.kill(pid2, 9)

    return true
end)


table.insert(steps, function()
    if draw_w ~= 100 then return end
    return true
end)

require("_runner").run_steps(steps)
