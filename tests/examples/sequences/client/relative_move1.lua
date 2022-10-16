 --DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE --DOC_GEN_OUTPUT
local module = ...
require("ruled.client")
local awful = {tag = require("awful.tag"), layout = require("awful.layout")}
awful.placement = require("awful.placement")
require("awful.ewmh")
screen[1]:fake_resize(0, 0, 800, 480)
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.tile)
local original_geo, geo

function awful.spawn(name)
    local c = client.gen_fake{class = name, name = name, x = 4, y=10, width = 60, height =50, screen=screen[1]}
    assert(c:geometry().x == 4)
    assert(c:geometry().y == 10)
    assert(c:geometry().width == 60)
    assert(c:geometry().height == 50)
end

--DOC_NEWLINE

module.add_event("Spawn a floating client.", function()
    --DOC_HIDE_END
    awful.spawn("")

    --DOC_NEWLINE

    client.get()[1].floating = true
    assert(client.get()[1]:geometry().x == 4) --DOC_HIDE
    assert(client.get()[1]:geometry().y == 10) --DOC_HIDE
    assert(client.get()[1]:geometry().width == 60) --DOC_HIDE
    assert(client.get()[1]:geometry().height == 50) --DOC_HIDE

    --DOC_NEWLINE

    --DOC_HIDE_START
end)

module.display_tags()

module.add_event("Move it.", function()
    --DOC_HIDE_END
    geo = client.get()[1]:geometry()
    print("Client geometry:", geo.x, geo.y, geo.width, geo.height)
    original_geo = geo --DOC_HIDE

    --DOC_NEWLINE

    client.get()[1]:relative_move(100, 100)
    --DOC_NEWLINE

    geo = client.get()[1]:geometry()
    print("Client geometry:", geo.x, geo.y, geo.width, geo.height)
    --DOC_NEWLINE

    --DOC_HIDE_START
    assert(original_geo.width == geo.width)
    assert(original_geo.height == geo.height)
    assert(original_geo.x == geo.x - 100)
    assert(original_geo.y == geo.y - 100)
    original_geo.x, original_geo.y = geo.x, geo.y
end)

module.display_tags()

module.add_event("Resize it.", function()
    --DOC_HIDE_END
    client.get()[1]:relative_move(nil, nil, 100, 100)
    --DOC_NEWLINE

    geo = client.get()[1]:geometry()
    print("Client geometry:", geo.x, geo.y, geo.width, geo.height)

    --DOC_HIDE_START
    assert(original_geo.width == geo.width - 100)
    assert(original_geo.height == geo.height - 100)
    assert(original_geo.x == geo.x)
    assert(original_geo.y == geo.y)
end)

module.display_tags()


module.execute { display_screen = true , display_clients     = true,
                 display_label  = false, display_client_name = true,
                 display_mouse  = false,
}
