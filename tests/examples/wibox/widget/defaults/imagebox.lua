local parent    = ... --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE

parent:add( --DOC_HIDE

           wibox.widget {
               image  = beautiful.awesome_icon,
               resize = false,
               widget = wibox.widget.imagebox
           }

           ) --DOC_HIDE
