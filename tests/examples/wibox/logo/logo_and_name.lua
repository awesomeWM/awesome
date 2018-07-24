--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require( "wibox" ) --DOC_HIDE
local assets    = require( "beautiful.theme_assets" ) --DOC_HIDE

local size = 128 --DOC_HIDE

parent:add( --DOC_HIDE

           wibox.widget {
               fit    = function() return size+20, size end,
               draw   = function(_, _, cr)
                   assets.gen_logo(cr, size, size, nil, "#535d6c")
                   cr:translate(size + size/16, 0)
                   assets.gen_awesome_name(cr, size+30, nil, "#535d6c", nil)
               end,
               widget = wibox.widget.base.make_widget
           }

           ) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
