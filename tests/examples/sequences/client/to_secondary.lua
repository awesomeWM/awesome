 --DOC_GEN_IMAGE
local module = ... --DOC_HIDE
require("ruled.client") --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout")} --DOC_HIDE
awful.placement = require("awful.placement") --DOC_HIDE
require("awful.ewmh") --DOC_HIDE
screen[1]:fake_resize(0, 0, 800, 480) --DOC_HIDE
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.tile) --DOC_HIDE

function awful.spawn(name) --DOC_HIDE
    client.gen_fake{class = name, name = name, x = 2094, y=10, width = 60, height =50, screen=screen[1]} --DOC_HIDE
end --DOC_HIDE


--DOC_NEWLINE

module.add_event("Spawn 5 clients in a `awful.layout.suit.tile` layout.", function() --DOC_HIDE
    -- Spawn a client on screen #3
    for i=1, 5 do
        awful.spawn("Client #"..i)
    end

    --DOC_NEWLINE

    client.get()[1]:activate {}
    client.get()[1].color = "#ff777733" --DOC_HIDE
end) --DOC_HIDE

--DOC_NEWLINE
module.display_tags() --DOC_HIDE

module.add_event("Call `:to_secondary_section()` on the 1st client.", function() --DOC_HIDE
    client.get()[1]:to_secondary_section()
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute { display_screen = true , display_clients     = true, --DOC_HIDE
                 display_label  = false, display_client_name = true, --DOC_HIDE
                 display_mouse  = true , --DOC_HIDE
} --DOC_HIDE
