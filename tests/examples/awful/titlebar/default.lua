--DOC_NO_USAGE
local place = require("awful.placement") --DOC_HIDE
local awful = { titlebar = require("awful.titlebar"), --DOC_HIDE
                button   = require("awful.button"), --DOC_HIDE
              } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local gears = {table = require("gears.table")} --DOC_HIDE

local c = client.gen_fake {hide_first=true} --DOC_HIDE
place.maximize(c, {honor_padding=true, honor_workarea=true}) --DOC_HIDE

    -- Create a titlebar for the client.
    -- By default, `awful.rules` will create one, but all it does is to call this
    -- function.

    local top_titlebar = awful.titlebar(c, {
        height    = 20,
        bg_normal = "#ff0000",
    })

    -- buttons for the titlebar
    local buttons = gears.table.join(
        awful.button({ }, 1, function()
            client.focus = c
            c:raise()
            awful.mouse.client.move(c)
        end),
        awful.button({ }, 3, function()
            client.focus = c
            c:raise()
            awful.mouse.client.resize(c)
        end)
    )

    top_titlebar : setup {
        { -- Left
            awful.titlebar.widget.iconwidget(c),
            buttons = buttons,
            layout  = wibox.layout.fixed.horizontal
        },
        { -- Middle
            { -- Title
                align  = "center",
                widget = awful.titlebar.widget.titlewidget(c)
            },
            buttons = buttons,
            layout  = wibox.layout.flex.horizontal
        },
        { -- Right
            awful.titlebar.widget.floatingbutton (c),
            awful.titlebar.widget.maximizedbutton(c),
            awful.titlebar.widget.stickybutton   (c),
            awful.titlebar.widget.ontopbutton    (c),
            awful.titlebar.widget.closebutton    (c),
            layout = wibox.layout.fixed.horizontal()
        },
        layout = wibox.layout.align.horizontal
    }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
