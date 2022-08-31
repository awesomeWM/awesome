--DOC_GEN_IMAGE
--DOC_NO_USAGE
--DOC_HIDE_START
local place = require("awful.placement")
local awful = { titlebar = require("awful.titlebar"),
                button   = require("awful.button"),
                tag      = require("awful.tag"),
              }
local wibox = require("wibox")
local gears = {table = require("gears.table")}

awful.tag.add("1", {screen=screen[1], selected = true})

local c = client.gen_fake {hide_first=true}
place.maximize(c, {honor_padding=true, honor_workarea=true})
--DOC_HIDE_END

    -- Create a titlebar for the client.
    -- By default, `ruled.client` will create one, but all it does is to call this
    -- function.

    local top_titlebar = awful.titlebar(c, {
        size      = 20,
        bg_normal = "#ff0000",
    })

    --DOC_NEWLINE

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

    --DOC_NEWLINE

    top_titlebar.widget = {
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

