 --DOC_GEN_IMAGE --DOC_HIDE_START --DOC_ASTERISK --DOC_NO_USAGE --DOC_GEN_OUTPUT
local module = ...
require("ruled.client")
local awful = {tag = require("awful.tag"), layout = require("awful.layout")}
awful.placement = require("awful.placement")
require("awful.ewmh")
screen[1]:fake_resize(0, 0, 800, 480)
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.tile)

function awful.spawn(name)
    client.gen_fake{class = name, name = name, x = 4, y=10, width = 60, height =50, screen=screen[1]}
end

--DOC_NEWLINE

module.add_event("Spawn a floating client.", function()
    --DOC_HIDE_END
    awful.spawn("")

    --DOC_NEWLINE

    client.get()[1].floating = true

    --DOC_NEWLINE
    --DOC_HIDE_START
end)

module.display_tags()

module.add_event("Move and resize it.", function()
    --DOC_HIDE_END
    client.get()[1]:geometry {
        x      = 200,
        y      = 200,
        width  = 300,
        height = 240
    }
    --DOC_NEWLINE

    -- It can also read the geometry.
    local geo = client.get()[1]:geometry()
    print("Client geometry:", geo.x, geo.y, geo.width, geo.height)

    --DOC_HIDE_START
end)

module.display_tags()

module.execute { display_screen = true , display_clients     = true,
                 display_label  = false, display_client_name = true,
                 display_mouse  = false,
}
