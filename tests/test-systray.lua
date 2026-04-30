local spawn = require("awful.spawn")
local wibox = require("wibox")
local beautiful = require("beautiful")

local steps, pid1, pid2, draw_w, wb, st = {}

table.insert(steps, function()
    screen[1].mywibox:remove()

    local widget = wibox.widget.base.make_widget()

    function widget:fit(_, w, h)
        return w, h
    end

    function widget:draw(_, _, width)
        draw_w = width
    end

    wb = wibox {
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

table.insert(steps, function()
    st.base_size = 20
    pid1 = spawn("env USE_TRAY_VISUAL_HINT=1 ./test-systray")
    return true
end)

local lgi_core = require("lgi.core")
local lgi_ffi = require("lgi.ffi")
local lgi_ti = lgi_ffi.types
local lgi_record = require("lgi.record")
local lgi_component = require("lgi.component")
local cairo = require("lgi").cairo
local wrapped_uchar = lgi_component.create(nil, lgi_record.struct_mt, "wrapped_uchar")
lgi_ffi.load_fields(wrapped_uchar, { { 'v', lgi_ti.uchar } })

table.insert(steps, function()
    if draw_w ~= 80 then return end
    local systray_surface_raw = awesome.systray_surface(20, 20)
    if systray_surface_raw then
        local src = cairo.Surface(systray_surface_raw, true)
        local s = cairo.ImageSurface("ARGB32", 20, 20)
        local cr = cairo.Context(s)
        cr:set_source_surface(src, 0, 0)
        cr:paint()
        -- Read the first pixel (a,r,g,b) as 4 8-bit integers.
        local data = s:get_data()
        local array = lgi_core.record.new(wrapped_uchar, data, false)
        local argb = {}
        for i = 0, 3 do
            argb[i + 1] = lgi_core.record.fromarray(array, i).v
        end
        -- Check with the pixel from test-systray.c
        return
            argb[1] == 0 and
            argb[2] == 153 and
            argb[3] == 255 and
            argb[4] == 255
    else
        print("Systray composition test is disabled.")
        return true
    end
end)

table.insert(steps, function()
    awesome.kill(pid1, 9)
    return true
end)

table.insert(steps, function()
    if draw_w ~= 100 then return end
    return true
end)

require("_runner").run_steps(steps)
