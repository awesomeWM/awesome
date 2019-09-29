 --DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_ASTERISK
local module = ... --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout")} --DOC_HIDE
awful.placement = require("awful.placement") --DOC_HIDE
require("awful.ewmh") --DOC_HIDE
screen[1]:fake_resize(0, 0, 1024/2, 768/2) --DOC_HIDE
screen.fake_add(1034/2, 0, 1024/2, 768/2).outputs = {["eVGA1"] = {mm_height=60/2, mm_width=80/2 }} --DOC_HIDE
screen.fake_add(2074/2, 0, 1024/2, 768/2).outputs = {["DVI1" ] = {mm_height=60/2, mm_width=80/2 }} --DOC_HIDE
screen.fake_add(3104/2, 0, 1024/2, 768/2).outputs = {["VGA1" ] = {mm_height=60/2, mm_width=80/2 }} --DOC_HIDE
awful.tag({ "1", "2", "3" }, screen[1], awful.layout.suit.corner.nw) --DOC_HIDE
awful.tag({ "1", "2", "3" }, screen[2], awful.layout.suit.corner.nw) --DOC_HIDE
awful.tag({ "1", "2", "3" }, screen[3], awful.layout.suit.corner.nw) --DOC_HIDE
awful.tag({ "1", "2", "3" }, screen[4], awful.layout.suit.corner.nw) --DOC_HIDE

local function spawn(name, s) --DOC_HIDE
    s = screen[s] --DOC_HIDE
    local c = client.gen_fake{ --DOC_HIDE
        class = name, name = name, x = s.geometry.x, y=s.geometry.x, --DOC_HIDE
        width = 640/2, height = 480/2, screen = s, floating = true --DOC_HIDE
    } --DOC_HIDE
    awful.placement.centered(c) --DOC_HIDE
end --DOC_HIDE

module.add_event("Spawn some applications", function() --DOC_HIDE
    spawn("maximize",1) --DOC_HIDE
    spawn("vertical",2) --DOC_HIDE
    spawn("horizontal",3) --DOC_HIDE
    spawn("fullscreen",4) --DOC_HIDE
    screen[3].clients[1].color = "#ff777733" --DOC_HIDE
    screen[3].clients[1].border_color = "#ff4444AA" --DOC_HIDE
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.add_event("Maximize 1 client per screen (differently", function() --DOC_HIDE
    screen[1].clients[1].maximized            = true
    screen[2].clients[1].maximized_vertical   = true
    screen[3].clients[1].maximized_horizontal = true
    screen[4].clients[1].fullscreen           = true
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute { display_screen = true , display_clients     = true, --DOC_HIDE
                 display_label  = false, display_client_name = true, --DOC_HIDE
                 display_mouse  = false, --DOC_HIDE
} --DOC_HIDE
