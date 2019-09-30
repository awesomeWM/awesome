 --DOC_GEN_IMAGE --DOC --DOC_NO_USAGE
local module = ... --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout") } --DOC_HIDE
screen[1]._resize {x = 0, width = 128, height = 96} --DOC_HIDE

module.add_event("Calling awful.tag.new", function() --DOC_HIDE
    assert(awful.layout.layouts[1]) --DOC_HIDE
    -- Calling awful.tag.new
    awful.tag({ "one", "two", "three", "four" }, screen[1])

    --DOC_NEWLINE
    screen[1].tags[3]:view_only()

end) --DOC_HIDE

module.display_tags() --DOC_HIDE

--DOC_NEWLINE

module.add_event("Select the next tag", function() --DOC_HIDE
    -- Select the next tag.
    awful.tag.viewnext()
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

--DOC_NEWLINE

module.add_event("Select the next tag (again)", function() --DOC_HIDE
    -- Select the next tag (again).
    awful.tag.viewnext()
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute() --DOC_HIDE
