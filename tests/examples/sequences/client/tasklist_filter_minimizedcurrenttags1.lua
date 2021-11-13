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

awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.corner.nw)

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
    client.gen_fake{
        class = name, name = name, x = 2094, y=10, width = 60, height =50, tags=args.tags, minimized=args.minimized
    }
end

--DOC_NEWLINE

module.add_event("Spawn some clients.", function()
    --DOC_HIDE_END

    -- Spawn 5 clients on screen 1.
    for k, t in ipairs(screen[1].tags) do
        for i= 1, 2 do
            awful.spawn("Client #"..(k-1)*2 + i, {tags = {t}, minimized = i == 1})
        end
    end

    --DOC_NEWLINE

    -- Selected some tags.
    screen[1].tags[3].selected = true
    screen[1].tags[5].selected = true

    --DOC_NEWLINE
    --DOC_HIDE_START

    client.get()[2]:activate {}
    client.get()[2].color = "#ff777733"
end)

--DOC_NEWLINE
module.display_tags()
module.display_widget(_tl, nil, 22)

module.add_event("Set the filter to minimizedcurrenttags.", function()
    --DOC_HIDE_END
    -- Set the filter to minimizedcurrenttags.
    tasklist.filter = awful.widget.tasklist.filter.minimizedcurrenttags
    --DOC_NEWLINE
    --DOC_HIDE_START
end)

module.display_widget(_tl, nil, 22)

module.execute { display_screen = false, display_clients     = true,
                 display_label  = false, display_client_name = true,
                 display_mouse  = true ,
}
