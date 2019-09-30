 --DOC_GEN_IMAGE --DOC --DOC_NO_USAGE
local module = ... --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout") } --DOC_HIDE
screen[1]._resize {x = 0, width = 128, height = 96} --DOC_HIDE

module.add_event("Calling awful.tag.new", function() --DOC_HIDE
    assert(awful.layout.layouts[1]) --DOC_HIDE
    -- Calling awful.tag.new
    awful.tag({ "one", "two", "three", "four" }, screen[1])

end) --DOC_HIDE

module.display_tags() --DOC_HIDE

--DOC_NEWLINE

module.add_event("Send the first tag to index 3", function() --DOC_HIDE
    -- Send the first tag to index 3.
    screen[1].tags[1].index = 3
end) --DOC_HIDE

module.display_tags() --DOC_HIDE
--DOC_NEWLINE

module.add_event("Send the fourth tag to index 1", function() --DOC_HIDE
    -- Send the first tag to index 3.
    screen[1].tags[4].index = 1
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute() --DOC_HIDE
