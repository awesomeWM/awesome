--DOC_GEN_IMAGE --DOC_HIDE_ALL
local awful =  { wallpaper = require("awful.wallpaper") }
local wibox = require("wibox")
local gears = {color = require("gears.color") }

screen._track_workarea = true
screen[1]._resize {x = 0, y = 0, width = 320, height = 196}
screen._add_screen {x = 330, y = 0, width = 320, height = 196}

require("_default_look")

   screen.connect_signal("request::wallpaper", function(s)
       awful.wallpaper {
           screen = s,
           honor_workarea = s.index == 2,
           bg     = "#222222",
           widget = wibox.widget {
                {
                    fit = function(_, width, height)
                        return width, height
                    end,
                    draw = function(_, _, cr, width, height)
                        local radius = math.min(width, height)/2
                        cr:arc(width/2, height/2, radius, 0, 2 * math.pi)
                        cr:set_source(gears.color {
                            type = "radial",
                            from  = { width/2, radius, 20  },
                            to    = { width/2, radius, 120 },
                            stops = {
                                { 0, "#0000ff" },
                                { 1, "#ff0000" },
                                { 1, "#000000" },
                            }
                        })
                        cr:fill()
                    end,
                    widget = wibox.widget.base.make_widget()
                },
                {
                    text   = "honor_workarea = " .. (s.index == 2 and "true" or "false"),
                    valign = "center",
                    halign = "center",
                    widget = wibox.widget.textbox,
                },
                widget = wibox.layout.stack
            }
        }
   end)

require("gears.timer").run_delayed_calls_now()

for s in screen do
    s:emit_signal("request::wallpaper", s)
end

require("gears.timer").run_delayed_calls_now()
