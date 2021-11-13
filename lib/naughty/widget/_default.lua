----------------------------------------------------------------------------
--- The default widget template for the notifications.
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2019 Emmanuel Lepage Vallee
-- @classmod naughty.widget._default
----------------------------------------------------------------------------

local wibox      = require("wibox")
local actionlist = require("naughty.list.actions")
local wtitle     = require("naughty.widget.title")
local wmessage   = require("naughty.widget.message")
local wicon      = require("naughty.widget.icon")
local wbg        = require("naughty.container.background")
local beautiful  = require("beautiful")
local dpi        = require("beautiful").xresources.apply_dpi

-- It is not worth doing a special widget for this.
local function notif_size()
    local constraint = wibox.container.constraint()
    constraint:set_strategy("max")
    constraint:set_width(beautiful.notification_max_width or dpi(500))

    rawset(constraint, "set_notification", function(_, notif)
        constraint._private.notification = setmetatable({notif}, {__mode = "v"})
        local s = false

        if notif.width and notif.width ~= beautiful.notification_max_width then
            constraint.width = notif.width
            s = true
        end
        if notif.height then
            constraint.height = notif.height
            s = true
        end

        constraint.strategy = s and "exact" or "max"
    end)

    rawset(constraint, "get_notification", function()
        return constraint._private.notification[1]
    end)

    return constraint
end

-- It is not worth doing a special widget for this either.
local function notif_margins()
    local margins = wibox.container.margin()
    margins:set_margins(beautiful.notification_margin or 4)

    rawset(margins, "set_notification", function(_, notif)
        if notif.margin then
            margins:set_margins(notif.margin)
        end
    end)

    return margins
end

-- Used as a fallback when no widget_template is provided, emulate the legacy
-- widget.
return {
    {
        {
            {
                {
                    wicon,
                    {
                        wtitle,
                        wmessage,
                        spacing = 4,
                        layout  = wibox.layout.fixed.vertical,
                    },
                    fill_space = true,
                    spacing    = 4,
                    layout     = wibox.layout.fixed.horizontal,
                },
                actionlist,
                spacing = 10,
                layout  = wibox.layout.fixed.vertical,
            },
            widget = notif_margins,
        },
        id     = "background_role",
        widget = wbg,
    },
    widget  = notif_size,
}
