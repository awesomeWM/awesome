 --DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local module = ...
require("ruled.client")
local awful = {
    tag    = require("awful.tag"),
    widget = {
        tasklist = require("awful.widget.tasklist")
    },
    button = require("awful.button"),
    layout = require("awful.layout")
}
require("awful.ewmh")
local wibox = require("wibox")
screen[1]:fake_resize(0, 0, 1024, 640)
screen.fake_add(1050, 0, 1024, 640).outputs = {["eVGA1"] = {mm_height=50, mm_width=80 }} --DOC_HIDE

awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.corner.nw)
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[2], awful.layout.suit.tile.left)


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

function awful.spawn(name, args)
    client.gen_fake{class = name, name = name, x = 2094, y=10, width = 60, height =50, screen=args.screen}
end


--DOC_NEWLINE

module.add_event("Spawn some clients.", function()
    --DOC_HIDE_END

    -- Spawn 5 clients on screen 1.
    for i= 1, 5 do
        awful.spawn("Client #"..i, {screen = screen[1]})
    end

    --DOC_NEWLINE

    -- Spawn 3 clients on screen 2.
    for i=1, 3 do
        awful.spawn("Client #"..(5+i), {screen = screen[2]})
    end

    --DOC_NEWLINE
    --DOC_HIDE_START

    client.get()[2]:activate {}
    client.get()[2].color = "#ff777733"
end)

--DOC_NEWLINE
module.display_tags()
module.display_widget(_tl, nil, 22)

module.add_event("Set the filter to allscreen.", function()
    --DOC_HIDE_END
    -- Set the filter to allscreen.
    tasklist.filter = awful.widget.tasklist.filter.allscreen
    --DOC_NEWLINE
    --DOC_HIDE_START
end)

module.display_widget(_tl, nil, 22)

module.add_event("Set a custom filter function.", function()
    --DOC_HIDE_END
    -- Create a pointless demo filter to only have clients
    -- with off numbers in the name. Because... example!
    tasklist.filter = function(c, screen) -- luacheck: no unused args
        return c.name:match("[13579]") and c or nil
    end
    --DOC_HIDE_START
end)

module.display_widget(_tl, nil, 22)

module.execute { display_screen = true , display_clients     = true,
                 display_label  = false, display_client_name = true,
                 display_mouse  = true ,
}
