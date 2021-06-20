--DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_HIDE_START
local awful = { titlebar = require("awful.titlebar"),
                button   = require("awful.button"),
              }
local beautiful = require("beautiful")
local wibox = require("wibox")
local gears = {table = require("gears.table")}

screen[1]._resize { width = 646, height = 182 }

local c

local clients = {
    client.gen_fake{ x = 10,  y = 10, height = 160, width = 200 },
    client.gen_fake{ x = 222, y = 10, height = 160, width = 200 },
    client.gen_fake{ x = 434, y = 10, height = 160, width = 200 },
}

local function setup(bar)
    bar:setup {
        awful.titlebar.widget.iconwidget(c),
        {
            align  = "center",
            widget = awful.titlebar.widget.titlewidget(c)
        },
        {
            awful.titlebar.widget.floatingbutton (c),
            awful.titlebar.widget.maximizedbutton(c),
            awful.titlebar.widget.stickybutton   (c),
            awful.titlebar.widget.ontopbutton    (c),
            awful.titlebar.widget.closebutton    (c),
            layout = wibox.layout.fixed.horizontal
        },
        layout = wibox.layout.align.horizontal
    }
end

c = clients[1]
setup(
--DOC_HIDE_END

    -- Create default titlebar
    awful.titlebar(c)

--DOC_HIDE_START
)

c = clients[2]
setup(
--DOC_HIDE_END

    -- Create titlebar on the client's bottom edge
    awful.titlebar(c, { position = "bottom" })

--DOC_HIDE_START
)

c = clients[3]
setup(
--DOC_HIDE_END

    -- Create titlebar with inverted colors
    awful.titlebar(c, { bg_normal = beautiful.fg_normal, fg_normal = beautiful.bg_normal })

--DOC_HIDE_START
)

return {hide_lines=true}
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
