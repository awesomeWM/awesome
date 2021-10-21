 --DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_HIDE_START --DOC_GEN_OUTPUT
local module = ...
local awful = {tag = require("awful.tag"), layout = require("awful.layout"),
    client = require("awful.client"), screen = require("awful.screen")}
require("awful.ewmh")
local beautiful = require("beautiful")
screen[1]._resize {x = 0, width = 640, height = 360}
awful.tag({ "one", "two", "three" }, screen[1], awful.layout.suit.tile)

local function color_focus()
    for _, c in ipairs(client.get()) do
        c.color = c.active and "#ff777733" or beautiful.bg_normal
    end
end

function awful.spawn(name)
    client.gen_fake{class = name, name = name, x = 10, y=10, width = 60, height =50}
end

--DOC_HIDE_END
   -- Print at which index each client is now at.
   local function print_indices()
       color_focus()
       local output = ""
       --DOC_NEWLINE
       for idx, c in ipairs(client.get()) do
            output = output .. c.name .. ":" .. idx .. ", "
       end

       --DOC_NEWLINE
       print(output)
   end
   --DOC_NEWLINE
--DOC_HIDE_START

module.add_event("Spawn some apps", function()
   for i = 1, 4 do
       awful.spawn("c"..i)
   end

   client.focus = client.get()[1]
   color_focus()
end)

module.display_tags()

module.add_event('Call `focus.byidx`', function()
   --DOC_HIDE_END

   print_indices()

   --DOC_NEWLINE
   print("Call focus.byidx")
   awful.client.focus.byidx(3, client.get()[1])

   print_indices()

   --DOC_HIDE_START
end)


--DOC_NEWLINE
module.display_tags()

module.add_event('Call `focus.byidx` again', function()
   --DOC_HIDE_END

   --DOC_NEWLINE
   print("Call focus.byidx")
   awful.client.focus.byidx(2, client.get()[4])

   print_indices()

   --DOC_HIDE_START
end)


module.display_tags()

module.execute { display_screen = true , display_clients     = true ,
                 display_label  = false, display_client_name = true }
