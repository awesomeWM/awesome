 --DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_ASTERISK
local module = ... --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout")} --DOC_HIDE
require("awful.ewmh") --DOC_HIDE
screen[1]._resize {x = 0, width = 800, height = 600} --DOC_HIDE
awful.tag({ "one", "two", "three" }, screen[1], awful.layout.suit.tile) --DOC_HIDE

function awful.spawn(name) --DOC_HIDE
    client.gen_fake{class = name, name = name, x = 10, y=10, width = 60, height =50} --DOC_HIDE
end --DOC_HIDE

module.add_event("Spawn some apps", function() --DOC_HIDE
    for _ = 1, 3 do
        awful.spawn("")
    end

    client.get()[1].color = "#ff777733" --DOC_HIDE

end) --DOC_HIDE

--DOC_NEWLINE
module.display_tags() --DOC_HIDE

module.add_event("Minimize the focused client", function() --DOC_HIDE
   client.get()[1].minimized = true
end) --DOC_HIDE

--DOC_NEWLINE
module.display_tags() --DOC_HIDE

module.add_event("Raise and focus", function() --DOC_HIDE
   -- That's the best way to unminimize if you also want to set the focus.
   client.get()[1]:activate {
       context = "unminimize",
       raise   = true,
   }
end) --DOC_HIDE


module.display_tags() --DOC_HIDE

module.execute { display_screen = true , display_clients     = true , --DOC_HIDE
                 display_label  = false, display_client_name = true } --DOC_HIDE
