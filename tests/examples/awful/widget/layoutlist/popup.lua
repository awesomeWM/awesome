local awful = { --DOC_HIDE --DOC_GEN_IMAGE --DOC_NO_USAGE
    popup = require("awful.popup"), --DOC_HIDE
    keygrabber = require("awful.keygrabber"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {layoutlist = require("awful.widget.layoutlist")} --DOC_HIDE
} --DOC_HIDE
local gears = { shape = require("gears.shape") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE
local modkey = "mod4" --DOC_HIDE

    local ll = awful.widget.layoutlist {
        source      = awful.widget.layoutlist.source.default_layouts, --DOC_HIDE
        base_layout = wibox.widget {
            spacing         = 5,
            forced_num_cols = 5,
            layout          = wibox.layout.grid.vertical,
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
    }

    --DOC_NEWLINE

    local layout_popup = awful.popup {
        widget = wibox.widget {
            ll,
            margins = 4,
            widget  = wibox.container.margin,
        },
        border_color = beautiful.border_color,
        border_width = beautiful.border_width,
        placement    = awful.placement.centered,
        ontop        = true,
        visible      = false,
        shape        = gears.shape.rounded_rect
    }

    --DOC_NEWLINE

    -- Make sure you remove the default `Mod4+Space` and `Mod4+Shift+Space`
    -- keybindings before adding this.
    awful.keygrabber {
        start_callback = function() layout_popup.visible = true  end,
        stop_callback  = function() layout_popup.visible = false end,
        export_keybindings = true,
        release_event = "release",
        stop_key = {"Escape", "Super_L", "Super_R"},
        keybindings = {
            {{ modkey          } , " " , function()
                awful.layout.set(gears.table.cycle_value(ll.layouts, ll.current_layout, 1))
            end},
            {{ modkey, "Shift" } , " " , function()
                awful.layout.set(gears.table.cycle_value(ll.layouts, ll.current_layout, -1), nil)
            end},
        }
    }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
