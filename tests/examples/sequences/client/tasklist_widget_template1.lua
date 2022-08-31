 --DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local module = ...
require("ruled.client")
local awful = {
    tag    = require("awful.tag"),
    widget = {
        tasklist = require("awful.widget.tasklist")
    },
    button = require("awful.button")
}
require("awful.ewmh")
local wibox = require("wibox")
screen[1]:fake_resize(0, 0, 800, 480)
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1])
local s = screen[1]
--DOC_HIDE_END

    local tasklist = awful.widget.tasklist {
        screen      = s,
        filter      = awful.widget.tasklist.filter.currenttags,
        base_layout = wibox.widget {
            spacing = 2,
            layout  = wibox.layout.fixed.horizontal,
        },
    }

    --DOC_NEWLINE
    --DOC_HIDE_START

local _tl = tasklist

function awful.spawn(name)
    client.gen_fake{class = name, name = name, x = 2094, y=10, width = 60, height =50, screen=screen[1]}
end


--DOC_NEWLINE

module.add_event("Spawn 5 clients.", function()

    -- Spawn 5 clients.
    for i=1, 5 do
        awful.spawn("Client #"..i)
    end

    client.get()[2]:activate {}
    client.get()[2].color = "#ff777733"
end)

--DOC_NEWLINE
module.display_widget(_tl, nil, 22)

module.add_event("Change the widget template.", function()
    --DOC_HIDE_END
    -- Change the widget template.
    tasklist.widget_template = {
        {
            {
                {
                    {
                        id     = "icon_role",
                        widget = wibox.widget.imagebox,
                    },
                    margins = 2,
                    widget  = wibox.container.margin,
                },
                {
                    id     = "text_role",
                    widget = wibox.widget.textbox,
                },
                layout = wibox.layout.fixed.horizontal,
            },
            left  = 10,
            right = 10,
            widget = wibox.container.margin
        },
        id     = "background_role",
        widget = wibox.container.background,
    }
    --DOC_NEWLINE
    --DOC_HIDE_START
end)

module.display_widget(_tl, nil, 22)

module.execute { display_screen = true , display_clients     = true,
                 display_label  = false, display_client_name = true,
                 display_mouse  = true ,
}
