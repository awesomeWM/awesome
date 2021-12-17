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

assert(awful.layout.layouts[1]) --DOC_HIDE
local some_layouts = {--DOC_HIDE
    awful.layout.suit.fair,--DOC_HIDE
    awful.layout.suit.fair,--DOC_HIDE
} --DOC_HIDE

    -- Calling awful.tag.new
    awful.tag({ "one", "two" }, screen[1], some_layouts)

assert(#screen[1].tags == 2) --DOC_HIDE
for k, t in ipairs(screen[1].tags) do --DOC_HIDE
    assert(t.layout and t.layout == some_layouts[k]) --DOC_HIDE
    assert(#t:clients() == 0) --DOC_HIDE
end --DOC_HIDE

--DOC_NEWLINE

module.add_event("Calling awful.tag.new and add some clients", function() --DOC_HIDE
    for _, t in ipairs(screen[1].tags) do--DOC_HIDE
        for _ = 1, 3 do --DOC_HIDE
            awful.spawn("xterm", {tag = t})--DOC_HIDE
        end --DOC_HIDE
        assert(#t:clients() == 3) --DOC_HIDE
    end --DOC_HIDE
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.add_event("Call `:clear()` on the first tag.", function() --DOC_HIDE
    -- Call :clear() on the first tag.
    screen[1].tags[1]:clear{}
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute {show_empty = true} --DOC_HIDE
