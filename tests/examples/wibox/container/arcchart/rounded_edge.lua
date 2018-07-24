--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

parent:add(wibox.widget {
           {
               {
                   markup = "<b>rounded_edge</b> = <i>false</i>",
                   widget = wibox.widget.textbox
               },
               {
                   {
                       {
                           colors = {
                               beautiful.bg_normal,
                               beautiful.bg_highlight,
                               beautiful.border_color,
                           },
                           values = {
                               1,
                               2,
                               3,
                           },
                           max_value    = 10,
                           min_value    = 0,
                           rounded_edge = false,
                           bg           = "#ff000033",
                           border_width = 0.5,
                           border_color = "#000000",
                           widget       = wibox.container.arcchart
                       },
                       margins = 2,
                       colors = {
                           beautiful.bg_normal,
                           beautiful.bg_highlight,
                           beautiful.border_color,
                       },
                       layout = wibox.container.margin
                   },
                   margins = 1,
                   color  = beautiful.border_color,
                   layout = wibox.container.margin,
               },
               layout = wibox.layout.fixed.vertical
           },
           {
               {
                   markup = "<b>rounded_edge</b> = <i>true</i>",
                   widget = wibox.widget.textbox
               },
               {
                   {
                       colors = {
                           beautiful.bg_normal,
                           beautiful.bg_highlight,
                           beautiful.border_color,
                       },
                       values = {
                           1,
                           2,
                           3,
                       },
                       max_value    = 10,
                       min_value    = 0,
                       rounded_edge = true,
                       bg           = "#ff000033",
                       border_width = 0.5,
                       border_color = "#000000",
                       widget       = wibox.container.arcchart
                   },
                   margins = 1,
                   color  = beautiful.border_color,
                   layout = wibox.container.margin,
               },
               layout = wibox.layout.fixed.vertical
           },
           layout = wibox.layout.flex.horizontal
       })

return 500, 60

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
