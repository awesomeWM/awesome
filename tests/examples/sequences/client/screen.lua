 --DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_ASTERISK
local module = ... --DOC_HIDE
require("ruled.client") --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout")} --DOC_HIDE
awful.placement = require("awful.placement") --DOC_HIDE
require("awful.ewmh") --DOC_HIDE
screen[1]:fake_resize(0, 0, 800, 480) --DOC_HIDE
screen.fake_add(830, 0, 800, 480).outputs = {["eVGA1"] = {mm_height=50, mm_width=80 }} --DOC_HIDE
screen.fake_add(1660, 0, 800, 480).outputs = {["DVI1" ] = {mm_height=50, mm_width=80 }} --DOC_HIDE
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.corner.nw) --DOC_HIDE
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[2], awful.layout.suit.corner.nw) --DOC_HIDE
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[3], awful.layout.suit.corner.nw) --DOC_HIDE

function awful.spawn(name) --DOC_HIDE
    client.gen_fake{class = name, name = name, x = 2094, y=10, width = 60, height =50, screen=screen[3]} --DOC_HIDE
end --DOC_HIDE


--DOC_NEWLINE

module.add_event("Spawn a client on screen #3", function() --DOC_HIDE
    -- Move the mouse to screen 3
    mouse.coords {x = 1800, y = 100 }
    assert(mouse.screen == screen[3]) --DOC_HIDE

    --DOC_NEWLINE

    -- Spawn a client on screen #3
    awful.spawn("firefox")

    assert(client.get()[1].screen == screen[3]) --DOC_HIDE
end) --DOC_HIDE

--DOC_NEWLINE
module.display_tags() --DOC_HIDE

module.add_event("Move to screen #2", function() --DOC_HIDE
    client.get()[1].screen = screen[2]
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute { display_screen = true , display_clients     = true, --DOC_HIDE
                 display_label  = false, display_client_name = true, --DOC_HIDE
                 display_mouse  = true , --DOC_HIDE
} --DOC_HIDE
