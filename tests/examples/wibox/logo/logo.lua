--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require( "wibox" ) --DOC_HIDE
local assets    = require( "beautiful.theme_assets" ) --DOC_HIDE

local size = 128 --DOC_HIDE

parent:add( --DOC_HIDE

           wibox.widget {
               fit    = function() return size, size end,
               draw   = function(_, _, cr)
                   assets.gen_logo(cr, size, size, nil, "#535d6c")
               end,
               widget = wibox.widget.base.make_widget
           }

           ) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
