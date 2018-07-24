--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_NO_USAGE --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE

parent:add( --DOC_HIDE
    wibox.widget {
        checked       = true,
        color         = beautiful.bg_normal,
        paddings      = 2,
        forced_width  = 20, --DOC_HIDE
        forced_height = 20, --DOC_HIDE
        check_shape   = function(cr, width, height)
            local rs = math.min(width, height)
            cr:move_to( 0  , 0  )
            cr:line_to( rs , 0  )
            cr:move_to( 0  , 0  )
            cr:line_to( 0  , rs )
            cr:move_to( 0  , rs )
            cr:line_to( rs , rs )
            cr:move_to( rs , 0  )
            cr:line_to( rs , rs )
            cr:move_to( 0  , 0  )
            cr:line_to( rs , rs )
            cr:move_to( 0  , rs )
            cr:line_to( rs , 0  )
        end,
        check_border_color = "#ff0000",
        check_color        = "#00000000",
        check_border_width = 1,
        widget             = wibox.widget.checkbox
    }
) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
