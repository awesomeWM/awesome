local parent    = ... --DOC_HIDE_ALL
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local Pango     = require("lgi").Pango --DOC_HIDE

-- Beautiful fake get_font --DOC_HIDE
local f = Pango.FontDescription.from_string("monospace 10") --DOC_HIDE
beautiful.get_font = function() return f end --DOC_HIDE

-- Fake beautiful theme --DOC_HIDE
beautiful.fg_focus = "#ff9800" --DOC_HIDE
beautiful.bg_focus = "#b9214f" --DOC_HIDE

local date = os.date("*t") --DOC_HIDE

    local w = wibox.widget {
        {
            {
                {
                    text = '{day='..date.day..', month='..date.month..',\n year='..date.year..'}',
                    align = 'center',
                    widget = wibox.widget.textbox
                },
                border_width = 2,
                bg = beautiful.bg_normal,
                widget  = wibox.container.background
            },
            wibox.widget.calendar.month({day=date.day, month=date.month, year=date.year}),
            spacing = 10,
            widget  = wibox.layout.fixed.vertical
        },
        {
            {
                {
                    text = '{month='..date.month..',\n year='..date.year..'}',
                    align = 'center',
                    widget = wibox.widget.textbox
                },
                border_width = 2,
                bg = beautiful.bg_normal,
                widget  = wibox.container.background
            },
            wibox.widget.calendar.month({month=date.month, year=date.year}),
            spacing = 10,
            widget  = wibox.layout.fixed.vertical
        },
        spacing = 20,
        widget  = wibox.layout.flex.horizontal
    }

parent:add(w) --DOC_HIDE
