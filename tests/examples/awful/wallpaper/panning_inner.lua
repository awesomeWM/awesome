--DOC_GEN_IMAGE --DOC_HIDE_ALL --DOC_NO_USAGE
local awful =  { wallpaper = require("awful.wallpaper") }
local wibox = require("wibox")
local gears = {color = require("gears.color") }

screen._track_workarea = true
screen[1]._resize {x = 0, y = 97, width = 96, height = 160}
screen._add_screen {x = 97, y = 129, width = 160, height = 96}
screen._add_screen {x = 258, y = 97,width = 96, height = 160}

screen._add_screen {x = 370, y = 0,width = 160, height = 96}
screen._add_screen {x = 402, y = 97,width = 96, height = 160}
screen._add_screen {x = 370, y = 258,width = 160, height = 96}

require("_default_look")

for i=0, 1 do
    awful.wallpaper {
        screens = {
            screen[i*3 + 1],
            screen[i*3 + 2],
            screen[i*3 + 3],
        },
        bg      = "#222222",
        panning_area = "inner",
        uncovered_areas_color = "#222222",
        widget  = wibox.widget {
            {
                fit = function(_, width, height)
                    return width, height
                end,
                draw = function(_, _, cr, width, height)
                    cr:set_source(gears.color("#0000ff"))
                    cr:line_to(width, height)
                    cr:line_to(width, 0)
                    cr:line_to(0, 0)
                    cr:close_path()
                    cr:fill()

                    cr:set_source(gears.color("#ff00ff"))
                    cr:move_to(0, 0)
                    cr:line_to(0, height)
                    cr:line_to(width, height)
                    cr:close_path()
                    cr:fill()
                end,
                widget = wibox.widget.base.make_widget()
            },
            {
                text = "Center",
                valign = "center",
                align  = "center",
                widget = wibox.widget.textbox,
            },
            widget = wibox.layout.stack
        }
    }
end

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
