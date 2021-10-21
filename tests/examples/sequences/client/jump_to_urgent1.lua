 --DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_HIDE_START
local module = ...
local awful = {tag = require("awful.tag"), layout = require("awful.layout"),
    client = require("awful.client")}
local beautiful = require("beautiful")
require("awful.ewmh")
screen[1]._resize {x = 0, width = 160, height = 90}
awful.tag({ "one", "two", "three" }, screen[1], awful.layout.suit.tile)
beautiful.bg_urgent = "#ff0000"

function awful.spawn(name, properties)
    client.gen_fake{class = name, name = name, x = 10, y=10, width = 60, height =50, tags = properties.tags}
end

module.add_event("Spawn some apps (urgent on tag #2)", function()
   for tag_idx = 1, 3 do
       for _ = 1, 3 do
           awful.spawn("", {tags = {screen[1].tags[tag_idx]}})
       end
   end

    client.get()[1].color = "#ff777733"
    screen[1].tags[2]:clients()[2].urgent = true

end)

--DOC_NEWLINE
module.display_tags()

module.add_event("Call `awful.client.urgent.jumpto()`", function()
   --DOC_HIDE_END
   awful.client.urgent.jumpto(false)
   --DOC_HIDE_START

   client.get()[1].color = beautiful.bg_normal
   screen[1].tags[2]:clients()[2].color = "#ff777733"
end)

module.display_tags()

module.execute { display_screen = false, display_clients     = true ,
                 display_label  = false, display_client_name = true }
