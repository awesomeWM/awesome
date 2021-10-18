 --DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_HIDE_START
local module = ...
local awful = {tag = require("awful.tag"), layout = require("awful.layout"),
    client = require("awful.client"), screen = require("awful.screen")}
require("awful.ewmh")
screen[1]._resize {x = 0, width = 640, height = 360}
screen.fake_add(660, 0, 640, 360)
awful.tag({ "one", "two", "three" }, screen[1], awful.layout.suit.tile)
awful.tag({ "one", "two", "three" }, screen[2], awful.layout.suit.tile)

function awful.spawn(name, properties)
    client.gen_fake{class = name, name = name, x = 10, y=10, width = 60, height =50, screen = properties.screen}
end

module.add_event("Spawn some apps", function()
   for s in screen do
       for i = 1, 4 do
           awful.spawn("c"..((s.index -1)*4 + i), {screen = s})
       end
   end

   client.focus = client.get()[3]
   client.focus.color = "#ff777733"
end)

module.display_tags()

module.add_event('Call `swap.bydirection` to the top', function()
   --DOC_HIDE_END

   --DOC_NEWLINE
   -- It will go up in the same column.
   awful.client.swap.bydirection("up", client.focus)

   --DOC_HIDE_START
end)


--DOC_NEWLINE
module.display_tags()

module.add_event('Call `swap.bydirection` to the right', function()
   --DOC_HIDE_END

   --DOC_NEWLINE
   -- Nothing happens because it cannot change screen.
   awful.client.swap.bydirection("right", client.focus)

   --DOC_HIDE_START
end)

--DOC_NEWLINE
module.display_tags()

module.add_event('Call `swap.bydirection` to the left', function()
   --DOC_HIDE_END

   --DOC_NEWLINE
   -- Moves to the first column.
   awful.client.swap.bydirection("left", client.focus)

   --DOC_HIDE_START
end)

module.display_tags()

module.execute { display_screen = true , display_clients     = true ,
                 display_label  = false, display_client_name = true }
