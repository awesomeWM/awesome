local parent    = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local Pango     = require("lgi").Pango --DOC_HIDE

-- Beautiful fake get_font --DOC_HIDE
local f = Pango.FontDescription.from_string("monospace 10") --DOC_HIDE
beautiful.get_font = function() return f end --DOC_HIDE

-- Fake beautiful theme --DOC_HIDE
beautiful.fg_focus = "#ff9800" --DOC_HIDE
beautiful.bg_focus = "#b9214f" --DOC_HIDE

    local cal = wibox.widget.calendar.month(os.date("*t"))

parent:add(cal) --DOC_HIDE
