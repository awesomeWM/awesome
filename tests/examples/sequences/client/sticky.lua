--DOC_GEN_IMAGE --DOC --DOC_NO_USAGE --DOC_ASTERISK
local module = ... --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout") } --DOC_HIDE
screen[1]._resize {x = 0, width = 128, height = 96} --DOC_HIDE


function awful.spawn(_) --DOC_HIDE
    local c = client.gen_fake{x = 10, y = 10, height=50, width=50} --DOC_HIDE
    c:tags{screen[1].tags[1]} --DOC_HIDE
end --DOC_HIDE

awful.tag({ "one", "two", "three", "four", "five" }, screen[1]) --DOC_HIDE

module.add_event("Add a client", function() --DOC_HIDE
    -- Add a client.
    awful.spawn("xterm")
    assert(#screen[1].clients == 1) --DOC_HIDE
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

--DOC_NEWLINE

module.add_event("Set sticky = true", function() --DOC_HIDE
    -- Set sticky = true
    screen[1].clients[1].sticky = true
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute {show_empty = true} --DOC_HIDE
