-- Some benchmarks that aren't really tests, but are included here anyway so
-- that we notice if they break.

local awful = require("awful")
local wibox = require("wibox")
local lgi = require("lgi")
local GLib = lgi.GLib
local cairo = lgi.cairo

local not_under_travis = not os.getenv("CI")

local measure, benchmark
do
    local timer_measure = GLib.Timer()
    measure = function(f, iter)
        timer_measure:start()
        for i = 1, iter do
            f()
        end
        local elapsed = timer_measure:elapsed()
        return elapsed / iter, elapsed
    end

    local timer_benchmark = GLib.Timer()
    benchmark = function(f, msg)
        timer_benchmark:start()
        local iters = 1
        local time_per_iter, time_total = measure(f, iters)
        -- To improve precision, we want to loop for this long
        local target_time = 1
        while time_total < target_time and not_under_travis do
            iters = math.ceil(target_time / time_per_iter)
            time_per_iter, time_total = measure(f, iters)
        end
        print(string.format("%20s: %-10.6g sec/iter (%3d iters, %.4g sec for benchmark)",
                msg, time_per_iter, iters, timer_benchmark:elapsed()))
    end
end

local function do_pending_repaint()
    awesome.emit_signal("refresh")
end

local function create_wibox()
    local img = cairo.ImageSurface(cairo.Format.ARGB32, 20, 20)

    -- Widgets that are aligned to the left
    local left_layout = wibox.layout.fixed.horizontal()
    left_layout:add(awful.widget.launcher({ image = img, command = "bash" }))
    left_layout:add(awful.widget.taglist(1, awful.widget.taglist.filter.all))
    left_layout:add(awful.widget.prompt())

    -- Widgets that are aligned to the right
    local right_layout = wibox.layout.fixed.horizontal()
    local textclock = awful.widget.textclock()
    right_layout:add(textclock)
    right_layout:add(awful.widget.layoutbox(1))

    -- Now bring it all together (with the tasklist in the middle)
    local layout = wibox.layout.align.horizontal()
    layout:set_left(left_layout)
    layout:set_middle(awful.widget.tasklist(1, awful.widget.tasklist.filter.currenttags))
    layout:set_right(right_layout)

    -- Create wibox
    local wb = wibox({ width = 1024, height = 20, screen = 1 })
    wb.visible = true
    wb:set_widget(layout)

    return wb, textclock
end

local wb, textclock = create_wibox()

local function relayout_textclock()
    textclock:emit_signal("widget::layout_changed")
    do_pending_repaint()
end

local function redraw_textclock()
    textclock:emit_signal("widget::redraw_needed")
    do_pending_repaint()
end

local function update_textclock()
    textclock:emit_signal("widget::updated")
    do_pending_repaint()
end

benchmark(create_wibox, "create wibox")
benchmark(update_textclock, "update textclock")
benchmark(relayout_textclock, "relayout textclock")
benchmark(redraw_textclock, "redraw textclock")

require("_runner").run_steps({ function() return true end })
