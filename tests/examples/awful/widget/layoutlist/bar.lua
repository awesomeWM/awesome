local awful = { --DOC_HIDE --DOC_GEN_IMAGE --DOC_NO_USAGE
    popup = require("awful.popup"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    wibar = require("awful.wibar"), --DOC_HIDE
    widget = {layoutlist = require("awful.widget.layoutlist"),--DOC_HIDE
              layoutbox= require("awful.widget.layoutbox")} --DOC_HIDE
} --DOC_HIDE
local gears = { shape = require("gears.shape") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

    -- Normally you would use your existing bars, but for this example, we add one
    local lb = awful.widget.layoutbox(screen[1])
    local l = wibox.layout.align.horizontal(nil, lb, nil)
    l.expand = "outside"
    awful.wibar { widget = l }

    --DOC_NEWLINE
    --DOC_HIDE cheat a bit, the show on widget click doesn't work with the shims

    local p = awful.popup {
        widget = wibox.widget {
            awful.widget.layoutlist {
                source      = awful.widget.layoutlist.source.default_layouts,
                screen      = 1,
                base_layout = wibox.widget {
                    spacing      = 5,
                    column_count = 3,
                    layout       = wibox.layout.grid.vertical,
                },
                widget_template = {
                    {
                        {
                            id            = 'icon_role',
                            forced_height = 22,
                            forced_width  = 22,
                            widget        = wibox.widget.imagebox,
                        },
                        margins = 4,
                        widget  = wibox.container.margin,
                    },
                    id              = 'background_role',
                    forced_width    = 24,
                    forced_height   = 24,
                    shape           = gears.shape.rounded_rect,
                    widget          = wibox.container.background,
                },
            },
            margins = 4,
            widget  = wibox.container.margin,
        },
        preferred_anchors = "middle",
        border_color      = beautiful.border_color,
        border_width      = beautiful.border_width,
        shape             = gears.shape.infobubble,
        placement    = function(d) return awful.placement.top(d, { --DOC_HIDE
            honor_workarea = true, --DOC_HIDE
        }) end, --DOC_HIDE
    }

    p:bind_to_widget(lb)



--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
