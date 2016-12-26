local parent    = ... --DOC_HIDE
local wibox     = require( "wibox" ) --DOC_HIDE
local assets    = require( "xresources.assets" ) --DOC_HIDE

parent:add( --DOC_HIDE

           wibox.widget {
               fit    = function() return 148, 128 end,
               draw   = function(_, _, cr)
                   assets.gen_logo(cr, 128, 128, nil, "#535d6c")
                   cr:translate(128 + 128/16, 0)
                   assets.gen_awesome_name(cr, 158, nil, "#535d6c", nil)
               end,
               widget = wibox.widget.base.make_widget
           }

           ) --DOC_HIDE
