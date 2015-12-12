-- Some benchmarks that aren't really tests, but are included here anyway so
-- that we notice if they break.

local awful = require("awful")
local GLib = require("lgi").GLib
local create_wibox = require("_wibox_helper").create_wibox

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

local function create_and_draw_wibox()
    create_wibox()
    do_pending_repaint()
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

local function e2e_tag_switch()
    awful.tag.viewnext()
    do_pending_repaint()
end

benchmark(create_and_draw_wibox, "create&draw wibox")
benchmark(update_textclock, "update textclock")
benchmark(relayout_textclock, "relayout textclock")
benchmark(redraw_textclock, "redraw textclock")
benchmark(e2e_tag_switch, "tag switch")

require("_runner").run_steps({ function() return true end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
