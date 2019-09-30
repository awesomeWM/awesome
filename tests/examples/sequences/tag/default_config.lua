 --DOC_GEN_IMAGE --DOC --DOC_NO_USAGE
local module = ... --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout") } --DOC_HIDE
screen[1]._resize {x = 0, width = 128, height = 96} --DOC_HIDE

module.add_event("Calling awful.tag.new", function() --DOC_HIDE
    assert(awful.layout.layouts[1]) --DOC_HIDE
    -- Calling awful.tag.new
    awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.layouts[1])

    assert(#screen[1].tags == 9) --DOC_HIDE
    for _, t in ipairs(screen[1].tags) do --DOC_HIDE
        assert(t.layout) --DOC_HIDE
    end --DOC_HIDE
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute() --DOC_HIDE
