 --DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local module = ...
require("ruled.client")
local awful = {
    tag    = require("awful.tag"),
    widget = {
        tasklist = require("awful.widget.tasklist")
    },
    layout = require("awful.layout"),
    button = require("awful.button")
}
require("awful.ewmh")
screen[1]:fake_resize(0, 0, 1024, 640)
screen.fake_add(1050, 0, 1024, 640).outputs = {["eVGA1"] = {mm_height=50, mm_width=80 }} --DOC_HIDE

awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.corner.nw)
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[2], awful.layout.suit.tile.left)

--DOC_HIDE_END

    local tasklist = awful.widget.tasklist {
        screen      = screen[1],
        filter      = awful.widget.tasklist.filter.currenttags,
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

module.display_tags()

--DOC_NEWLINE
module.display_widget(_tl, nil, 22)

module.add_event("Change the tasklist screen.", function()
    --DOC_HIDE_END
    -- Change the tastlist screen.
    tasklist.screen = screen[2]
    --DOC_NEWLINE
    --DOC_HIDE_START
end)

module.display_widget(_tl, nil, 22)

module.execute { display_screen = true , display_clients     = true,
                 display_label  = false , display_client_name = true,
                 display_mouse  = true ,
}
