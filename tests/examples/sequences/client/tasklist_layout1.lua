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
        buttons     = {
            awful.button({ }, 1, function (c)
                c:activate {
                    context = "tasklist",
                    action  = "toggle_minimization"
                }
            end),
            awful.button({ }, 3, function() awful.menu.client_list { theme = { width = 250 } } end),
            awful.button({ }, 4, function() awful.client.focus.byidx(-1) end),
            awful.button({ }, 5, function() awful.client.focus.byidx( 1) end),
        },
    }

    --DOC_NEWLINE

    tasklist:connect_signal("property::count", function(self)
        local count = self.count
        --DOC_NEWLINE
        if count > 5 and not self.is_grid then
            self.base_layout = wibox.widget {
                forced_num_rows = 2,
                homogeneous     = true,
                expand          = true,
                spacing         = 2,
                layout          = wibox.layout.grid.horizontal
            }
            --DOC_NEWLINE
            self.is_grid = true
        elseif count <= 5 and self.is_grid then
            self.base_layout = wibox.widget {
                spacing = 2,
                layout  = wibox.layout.fixed.horizontal
            }
            --DOC_NEWLINE
            self.is_grid = false
        end
    end)

    --DOC_NEWLINE
    --DOC_HIDE_START

local _tl = tasklist

function awful.spawn(name)
    client.gen_fake{class = name, name = name, x = 2094, y=10, width = 60, height =50, screen=screen[1]}
end


--DOC_NEWLINE

module.add_event("Spawn 5 clients.", function()
    --DOC_HIDE_END

    -- Spawn 5 clients.
    for i=1, 5 do
        awful.spawn("Client #"..i)
    end

    --DOC_NEWLINE
    --DOC_HIDE_START

    client.get()[2]:activate {}
    client.get()[2].color = "#ff777733"
end)

--DOC_NEWLINE
module.display_widget(_tl, nil, 22)

module.add_event("Spawn another client.", function()
    --DOC_HIDE_END
    -- Spawn another client.
    awful.spawn("Client #6")
    --DOC_NEWLINE
    --DOC_HIDE_START
end)

module.display_widget(_tl, nil, 44)

module.add_event("Kill 3 clients.", function()
    --DOC_HIDE_END
    -- Kill 3 clients.
    for _=1, 3 do
        client.get()[1]:kill()
    end
    --DOC_HIDE_START
end)

module.display_widget(_tl, nil, 22)

module.execute { display_screen = true , display_clients     = true,
                 display_label  = false, display_client_name = true,
                 display_mouse  = true ,
}
