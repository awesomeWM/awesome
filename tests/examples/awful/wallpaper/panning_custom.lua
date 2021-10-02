--DOC_GEN_IMAGE --DOC_GEN_OUTPUT --DOC_NO_USAGE
--DOC_HIDE_START
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

local walls = {}

--DOC_HIDE_END

   local function custom_panning_area(wallpaper)
        return {
            x      = wallpaper.screens[1].geometry.x + 50,
            y      = wallpaper.screens[2].geometry.y + 50,
            width  = 96,
            height = 96,
        }
   end

--DOC_HIDE_START
for i=0, 1 do
    walls[i+1] = awful.wallpaper {
        screens = {
            screen[i*3 + 1],
            screen[i*3 + 2],
            screen[i*3 + 3],
        },
        bg      = "#222222",
        panning_area = custom_panning_area,
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

require("gears.timer").run_delayed_calls_now()


--DOC_HIDE_END
--DOC_NEWLINE

   -- Areas due to the padding and the wibar (workarea).
   for k, wall in ipairs(walls) do
       for _, area in ipairs(wall.uncovered_areas) do
            print("Uncovered wallpaper #".. k .." area:", area.x, area.y, area.width, area.height)
       end
   end
