--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local gears = {color = require("gears.color") } --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE
local s = screen[1] --DOC_HIDE

   awful.wallpaper {
       screen = s,
       widget = wibox.widget {
            fit = function(_, width, height)
                return width, height
            end,
            draw = function(_, _, cr, width, height)
                cr:set_source(gears.color {
                    type  = 'linear',
                    from  = { 0, 0      },
                    to    = { 0, height },
                    stops = {
                        { 0   , '#030d27' },
                        { 0.75, '#3a183f' },
                        { 0.75, '#000000' },
                        { 1   , '#222222' }
                    }
                })
                cr:paint()

                -- Clip the first 33% of the screen
                cr:rectangle(0,0, width, height/3)

                --DOC_NEWLINE

                -- Clip-out some increasingly large sections of add the sun "bars"
                for i=0, 6 do
                    cr:rectangle(0, height*.28 + i*(height*.055 + i/2), width, height*.055)
                end
                cr:clip()

                --DOC_NEWLINE

                -- Draw the sun
                cr:set_source(gears.color {
                    type  = 'linear' ,
                    from  = { 0, 0      },
                    to    = { 0, height },
                    stops = {
                        { 0, '#f0d64f' },
                        { 1, '#e484c6' }
                    }
                })
                cr:arc(width/2, height/2, height*.35, 0, math.pi*2)
                cr:fill()

                --DOC_NEWLINE

                -- Draw the grid
                local lines = width/8
                cr:reset_clip()
                cr:set_line_width(0.5)
                cr:set_source(gears.color("#8922a3"))

                --DOC_NEWLINE

                for i=1, lines do
                    cr:move_to((-width) + i* math.sin(i * (math.pi/(lines*2)))*30, height)
                    cr:line_to(width/4 + i*((width/2)/lines), height*0.75 + 2)
                    cr:stroke()
                end

                --DOC_NEWLINE

                for i=1, 5 do
                    cr:move_to(0, height*0.75 + i*10 + i*2)
                    cr:line_to(width, height*0.75 + i*10 + i*2)
                    cr:stroke()
                end
            end,
       }
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
