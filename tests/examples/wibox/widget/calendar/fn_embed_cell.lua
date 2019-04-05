--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local gears     = require("gears") --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local Pango     = require("lgi").Pango --DOC_HIDE
require("_date") --DOC_HIDE

-- Beautiful fake get_font --DOC_HIDE
local f = Pango.FontDescription.from_string("monospace 10") --DOC_HIDE
beautiful.get_font = function() return f end --DOC_HIDE

-- Fake beautiful theme --DOC_HIDE
beautiful.fg_focus = "#ff9800" --DOC_HIDE
beautiful.bg_focus = "#b9214f" --DOC_HIDE

    local styles = {}
    local function rounded_shape(size, partial)
        if partial then
            return function(cr, width, height)
                       gears.shape.partially_rounded_rect(cr, width, height,
                            false, true, false, true, 5)
                   end
        else
            return function(cr, width, height)
                       gears.shape.rounded_rect(cr, width, height, size)
                   end
        end
    end
    styles.month   = { padding      = 5,
                       bg_color     = "#555555",
                       border_width = 2,
                       shape        = rounded_shape(10)
    }
    styles.normal  = { shape    = rounded_shape(5) }
    styles.focus   = { fg_color = "#000000",
                       bg_color = "#ff9800",
                       markup   = function(t) return '<b>' .. t .. '</b>' end,
                       shape    = rounded_shape(5, true)
    }
    styles.header  = { fg_color = "#de5e1e",
                       markup   = function(t) return '<b>' .. t .. '</b>' end,
                       shape    = rounded_shape(10)
    }
    styles.weekday = { fg_color = "#7788af",
                       markup   = function(t) return '<b>' .. t .. '</b>' end,
                       shape    = rounded_shape(5)
    }

    local function decorate_cell(widget, flag, date)
        if flag=="monthheader" and not styles.monthheader then
            flag = "header"
        end
        local props = styles[flag] or {}
        if props.markup and widget.get_text and widget.set_markup then
            widget:set_markup(props.markup(widget:get_text()))
        end

        -- Change bg color for weekends
        local d = {year=date.year, month=(date.month or 1), day=(date.day or 1)}
        local weekday = tonumber(os.date("%w", os.time(d)))
        local default_bg = (weekday==0 or weekday==6) and "#232323" or "#383838"

        local ret = wibox.widget {
            {
                widget,
                margins = (props.padding or 2) + (props.border_width or 0),
                widget  = wibox.container.margin
            },
            shape        = props.shape,
            border_color = props.border_color or "#b9214f",
            border_width = props.border_width or 0,
            fg           = props.fg_color or "#999999",
            bg           = props.bg_color or default_bg,
            widget       = wibox.container.background
        }
        return ret
    end

    local cal = wibox.widget {
        date     = os.date("*t"),
        fn_embed = decorate_cell,
        widget   = wibox.widget.calendar.month
    }

parent:add(cal) --DOC_HIDE
