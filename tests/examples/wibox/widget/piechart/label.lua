--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

parent:add(wibox.widget {
           {
               {
                   markup = "<b>display_labels</b> = <i>false</i>",
                   widget = wibox.widget.textbox
               },
               {
                   {
                       data_list = {
                           { 'L1', 100 },
                           { 'L2', 200 },
                           { 'L3', 300 },
                       },
                       border_width = 1,
                       forced_height = 50,
                       forced_width  = 100,
                       display_labels = false,
                       colors = {
                           beautiful.bg_normal,
                           beautiful.bg_highlight,
                           beautiful.border_color,
                       },
                       widget = wibox.widget.piechart
                   },
                   margins = 1,
                   color  = beautiful.border_color,
                   layout = wibox.container.margin,
               },
               layout = wibox.layout.fixed.vertical
           },
           {
               {
                   markup = "<b>display_labels</b> = <i>true</i>",
                   widget = wibox.widget.textbox
               },
               {
                   {
                       data_list = {
                           { 'L1', 100 },
                           { 'L2', 200 },
                           { 'L3', 300 },
                       },
                       border_width = 1,
                       forced_height = 50,
                       forced_width  = 100,
                       display_labels = true,
                       colors = {
                           beautiful.bg_normal,
                           beautiful.bg_highlight,
                           beautiful.border_color,
                       },
                       widget = wibox.widget.piechart
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
