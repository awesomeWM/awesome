 --DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_HIDE_START
local module = ...
local awful = {tag = require("awful.tag"), layout = require("awful.layout"),
    client = require("awful.client")}
require("awful.ewmh")
screen[1]._resize {x = 0, width = 640, height = 360} --DOC_HIDE

awful.tag({ "one", "two", "three" }, screen[1], awful.layout.suit.tile)

function awful.spawn(name)
    client.gen_fake{class = name, name = name, x = 10, y=10, width = 60, height =50}
end

module.add_event("Spawn some apps", function()
   --DOC_HIDE_END
   for i = 1, 5 do
       awful.spawn("c"..i)
   end
   --DOC_HIDE_START
end)

module.display_tags()

module.add_event("Minimize everything", function()
   --DOC_NEWLINE
   --DOC_HIDE_END
   -- Minimize everything.
   for _, c in ipairs(client.get()) do
       c.minimized = true
   end
    --DOC_HIDE_START
end)

module.display_tags()

module.add_event("Restore a client", function()
   local real_pairs = pairs

   -- Mock for reproducible builds
   rawset(screen[1], "selected_tags", screen[1].selected_tags)

   local ret = client.get()[1]
   pairs = function(t, ...) --luacheck: globals pairs
       local ret2 = ret
       ret = nil

       -- Unmock.
       if not ret2 then
           pairs = real_pairs  --luacheck: globals pairs
           return pairs(t,  ...)
       end

       return function() ret2, ret = ret, nil; return ret2 and 1 or nil, ret2 end, t
   end

   --DOC_NEWLINE
   --DOC_HIDE_END
   --DOC_NEWLINE
   -- Restore a random client.
   awful.client.restore()
   --DOC_HIDE_START
end)

module.display_tags()

module.execute { display_screen = true , display_clients     = true ,
                 display_label  = false, display_client_name = true }
