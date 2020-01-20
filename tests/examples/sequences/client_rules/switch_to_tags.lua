 --DOC_GEN_IMAGE --DOC_NO_USAGE
local module = ... --DOC_HIDE
local ruled = {client = require("ruled.client") } --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout")} --DOC_HIDE
require("awful.ewmh") --DOC_HIDE
screen[1]._resize {x = 0, width = 128, height = 96} --DOC_HIDE
awful.tag({ "one", "two", "three", "four", "five" }, screen[1], awful.layout.layouts[1]) --DOC_HIDE


function awful.spawn(name) --DOC_HIDE
    client.gen_fake{class = name, name = name, x = 10, y=10, width = 60, height =50} --DOC_HIDE
end --DOC_HIDE

module.display_tags() --DOC_HIDE

client.connect_signal("request::rules", function() --DOC_HIDE
    -- Select tag by object reference:
    ruled.client.append_rule {
        rule = { class = "firefox" },
        properties = {
            tag            = screen[1].tags[4],
            switch_to_tags = true
        }
    }

end) --DOC_HIDE

client.emit_signal("request::rules") --DOC_HIDE

--DOC_NEWLINE

module.add_event("Spawn xterm", function() --DOC_HIDE
    -- Spawn firefox
    awful.spawn("firefox")
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute { display_screen = false, display_clients     = true , --DOC_HIDE
                 display_label  = false, display_client_name = true } --DOC_HIDE
