 --DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_GEN_OUTPUT --DOC_HIDE_START
local module = ...
local awful = {tag = require("awful.tag"), layout = require("awful.layout")}
local beautiful = require("beautiful")
require("awful.ewmh")
screen[1]._resize {x = 0, width = 160, height = 90}
awful.tag({ "one", "two", "three" }, screen[1], awful.layout.suit.tile)

function awful.spawn(name, properties)
    client.gen_fake{class = name, name = name, x = 10, y=10, width = 60, height =50, tags = properties.tags}
end

local function color_focus()
    for _, c in ipairs(client.get()) do
        c.color = c.active and "#ff777733" or beautiful.bg_normal
    end
end

module.add_event("Spawn some apps", function()
   for tag_idx = 1, 3 do
       for i = 1, 3 do
           awful.spawn("c"..((tag_idx-1)*3+i), {tags = {screen[1].tags[tag_idx]}})
       end
   end

   client.get()[2]:activate{}

   color_focus()
end)

--DOC_NEWLINE
module.display_tags()

module.add_event('Activate "c8"', function()
   -- Mitigate a bug in the shims.
   client.get()[8]:activate {
        switch_to_tag = true,
        raise         = true,
        context       = "somet_reason",
   }

   --DOC_HIDE_END

   client.get()[8]:activate {
        switch_to_tag = true,
        raise         = true,
        context       = "somet_reason",
   }

   --DOC_NEWLINE

   -- Since this isnt denied by any permission, it will be true.
   print(
       "Confirm:", client.get()[8].active, client.focus == client.get()[8]
   )

   --DOC_HIDE_START
   color_focus()
end)

module.display_tags()

module.execute { display_screen = false, display_clients     = true ,
                 display_label  = false, display_client_name = true }
