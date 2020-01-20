 --DOC_GEN_IMAGE --DOC_NO_USAGE
local module = ... --DOC_HIDE
local ruled = {client = require("ruled.client") } --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout")} --DOC_HIDE
awful.placement = require("awful.placement") --DOC_HIDE
require("awful.ewmh") --DOC_HIDE
screen[1]:fake_resize(0, 0, 1280, 720) --DOC_HIDE
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.suit.corner.nw) --DOC_HIDE

function awful.spawn(name) --DOC_HIDE
    client.gen_fake{class = name, name = name, x = 10, y=10, width = 60, height =50} --DOC_HIDE
end --DOC_HIDE

client.connect_signal("request::rules", function() --DOC_HIDE
   ruled.client.append_rule {
       rule = { class = "mplayer" },
       properties = {
           floating  = true,
           placement = awful.placement.centered,
           width     = 640,
           height    = 480,
       },
   }
end) --DOC_HIDE

client.emit_signal("request::rules") --DOC_HIDE

--DOC_NEWLINE

module.add_event("Spawn mplayer", function() --DOC_HIDE
   -- Spawn mplayer
   awful.spawn("mplayer")

   for _, c in ipairs(client.get()) do --DOC_HIDE
       assert(#c:tags() == 1) --DOC_HIDE
       assert(c.floating == (c.class == "mplayer")) --DOC_HIDE
   end --DOC_HIDE
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute { display_screen = true , display_clients     = true , --DOC_HIDE
                 display_label  = false, display_client_name = true } --DOC_HIDE
