 --DOC_GEN_IMAGE --DOC_HIDE_START --DOC_ASTERISK --DOC_NO_USAGE
local module = ...
require("ruled.client")
local awful = {tag = require("awful.tag"), layout = require("awful.layout")}
awful.placement = require("awful.placement")
require("awful.ewmh")
screen[1]:fake_resize(0, 0, 800, 480)
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.tile)

function awful.spawn(name)
    client.gen_fake{class = name, name = name, x = 2094, y=10, width = 60, height =50, screen=screen[1]}
end

--DOC_NEWLINE

module.add_event("Spawn 5 clients in a `awful.layout.suit.tile` layout.", function()
    --DOC_HIDE_END
    -- Spawn a client on screen #3
    for i=1, 5 do
        awful.spawn("Client #"..i)
    end

    --DOC_NEWLINE

    client.get()[5]:activate {}
    client.get()[5].color = "#ff777733" --DOC_HIDE

    --DOC_NEWLINE
    --DOC_HIDE_START
end)

module.display_tags()

module.add_event("Kill the the 4th and 5th clients.", function()
    --DOC_HIDE_END
    local c4, c5 =  client.get()[4], client.get()[5]

    --DOC_NEWLINE
    -- Kill the clients.
    c4:kill()
    c5:kill()
    --DOC_HIDE_START
end)

module.display_tags()

module.execute { display_screen = true , display_clients     = true,
                 display_label  = false, display_client_name = true,
                 display_mouse  = false,
}
