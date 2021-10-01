--DOC_GEN_IMAGE --DOC_HIDE_ALL
local awful =  { wallpaper = require("awful.wallpaper") }
local wibox = require("wibox")
local gears = {color = require("gears.color") }

screen._track_workarea = true
screen[1]._resize {x = 0, y = 0, width = 160, height = 96}
screen._add_screen {x = 161, y = 0, width = 160, height = 96}
screen._add_screen {x = 322, y = 0, width = 160, height = 96}

require("_default_look")

       local wall = awful.wallpaper {
           screen = screen[1],
           bg     = "#222222",
           widget = wibox.widget {
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

   wall:add_screen(screen[2])

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
