 --DOC_GEN_IMAGE --DOC_NO_USAGE
local module = ... --DOC_HIDE
local ruled = {client = require("ruled.client") } --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout")} --DOC_HIDE
awful.placement = require("awful.placement") --DOC_HIDE
require("awful.ewmh") --DOC_HIDE
screen[1]:fake_resize(0, 0, 1024, 768) --DOC_HIDE
screen.fake_add(1034, 0, 1024, 768).outputs = {["eVGA1"] = {mm_height=60, mm_width=80 }} --DOC_HIDE
screen.fake_add(2074, 0, 1024, 768).outputs = {["DVI1" ] = {mm_height=60, mm_width=80 }} --DOC_HIDE
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.corner.nw) --DOC_HIDE
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[2], awful.layout.suit.corner.nw) --DOC_HIDE
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[3], awful.layout.suit.corner.nw) --DOC_HIDE

function awful.spawn(name) --DOC_HIDE
    client.gen_fake{class = name, name = name, x = 10, y=10, width = 60, height =50} --DOC_HIDE
end --DOC_HIDE

    mouse.coords {x = 2500, y = 100 }
assert(mouse.screen == screen[3]) --DOC_HIDE

--DOC_NEWLINE

client.connect_signal("request::rules", function() --DOC_HIDE
    -- Screen by IDs:
    ruled.client.append_rule {
        rule_any    = {
            class = {"firefox"}
        },
        properties = {
            screen = 2,
            tag    = "1", --DOC_HIDE
            width  = 640, height =480, floating = true, --DOC_HIDE
        },
    }
--DOC_NEWLINE
    -- Screen by object:
    ruled.client.append_rule {
        rule_any    = {
            class = {"thunderbird"}
        },
        properties = {
            screen = mouse.screen,
            tag    = "1", --DOC_HIDE
            width  = 640, height =480, floating = true, --DOC_HIDE
        },
    }
--DOC_NEWLINE
    -- Screen by output name:
    ruled.client.append_rule {
        rule_any    = {
            class = {"xterm"}
        },
        properties = {
            screen = "LVDS1",
            tag    = "1", --DOC_HIDE
            width  = 640, height =480, floating = true, --DOC_HIDE
        },
    }

end) --DOC_HIDE

client.emit_signal("request::rules") --DOC_HIDE

--DOC_NEWLINE

module.add_event("Spawn some applications", function() --DOC_HIDE
    -- Spawn firefox, thunderbird and xterm
    awful.spawn("firefox")
    awful.spawn("thunderbird")
    awful.spawn("xterm")

    assert(client.get()[1].screen == screen[2]) --DOC_HIDE
    assert(client.get()[2].screen == screen[3]) --DOC_HIDE
    assert(client.get()[3].screen == screen[1]) --DOC_HIDE
    assert(client.get()[1]:tags()[1] == screen[2].selected_tag) --DOC_HIDE
    assert(client.get()[2]:tags()[1] == screen[3].selected_tag) --DOC_HIDE
    assert(client.get()[3]:tags()[1] == screen[1].selected_tag) --DOC_HIDE
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute { display_screen = true , display_clients     = true, --DOC_HIDE
                 display_label  = true , display_client_name = true, --DOC_HIDE
                 display_mouse  = true , --DOC_HIDE
} --DOC_HIDE
