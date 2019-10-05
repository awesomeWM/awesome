 --DOC_GEN_IMAGE --DOC --DOC_NO_USAGE
local module = ... --DOC_HIDE
local awful = {tag = require("awful.tag"), --DOC_HIDE
    layout = require("awful.layout"), --DOC_HIDE
} --DOC_HIDE
screen[1]._resize {x = 0, width = 128, height = 96} --DOC_HIDE

function awful.spawn(_, args) --DOC_HIDE
    local c = client.gen_fake{} --DOC_HIDE
    c:tags({args.tag}) --DOC_HIDE
    assert(#c:tags() == 1) --DOC_HIDE
    assert(c:tags()[1] == args.tag) --DOC_HIDE
end --DOC_HIDE

module.add_event("Calling awful.tag.new", function() --DOC_HIDE
    assert(awful.layout.layouts[1]) --DOC_HIDE
    local some_layouts = {
        awful.layout.suit.fair,
        awful.layout.suit.spiral,
        awful.layout.suit.spiral.dwindle,
        awful.layout.suit.magnifier,
        awful.layout.suit.corner.nw,
    }

    --DOC_NEWLINE

    -- Calling awful.tag.new
    awful.tag({ "one", "two", "three", "four", "five" }, screen[1], some_layouts)

    assert(#screen[1].tags == 5) --DOC_HIDE
    for k, t in ipairs(screen[1].tags) do --DOC_HIDE
        assert(t.layout and t.layout == some_layouts[k]) --DOC_HIDE
        assert(#t:clients() == 0) --DOC_HIDE
    end --DOC_HIDE
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

--DOC_NEWLINE

module.add_event("Add some clients", function() --DOC_HIDE
    -- Add some clients
    for _, t in ipairs(screen[1].tags) do
        for _ = 1, 5 do
            awful.spawn("xterm", {tag = t})
        end
        assert(#t:clients() == 5) --DOC_HIDE
    end
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute {show_empty = true} --DOC_HIDE
