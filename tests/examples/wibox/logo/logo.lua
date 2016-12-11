local parent    = ... --DOC_HIDE
local wibox     = require( "wibox" ) --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local assets    = require( "xresources.assets" ) --DOC_HIDE
local color     = require( "gears.color" ) --DOC_HIDE

local size = 128 --DOC_HIDE

parent:add( --DOC_HIDE

wibox.widget {
    fit    = function() return 128, 128 end,
    draw   = function(_, _, cr)
        assets.gen_logo(cr, 128, 128, nil, "#535d6c")
    end,
    widget = wibox.widget.base.make_widget
}

) --DOC_HIDE
