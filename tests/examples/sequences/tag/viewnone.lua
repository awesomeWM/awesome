 --DOC_GEN_IMAGE --DOC --DOC_NO_USAGE
local module = ... --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout") } --DOC_HIDE
screen[1]._resize {x = 0, width = 128, height = 96} --DOC_HIDE

module.add_event("Calling awful.tag.new", function() --DOC_HIDE
    assert(awful.layout.layouts[1]) --DOC_HIDE
    -- Calling awful.tag.new
    awful.tag({ "one", "two", "three", "four" }, screen[1])

    --DOC_NEWLINE

end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.add_event("Manually select some tags", function() --DOC_HIDE
    -- Manually select some tags (tag 1 was auto selected).
    screen[1].tags[3].selected = true
    screen[1].tags[4].selected = true
end)--DOC_HIDE

module.display_tags() --DOC_HIDE

--DOC_NEWLINE

module.add_event("Deselect all tags", function() --DOC_HIDE
    -- Deselect all tags.
    awful.tag.viewnone()
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute{show_code_pointer = true} --DOC_HIDE
