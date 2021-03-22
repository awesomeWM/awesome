 --DOC_GEN_IMAGE --DOC --DOC_NO_USAGE
local module = ... --DOC_HIDE
local awful = {tag = require("awful.tag"), layout = require("awful.layout") } --DOC_HIDE
screen[1]._resize {x = 0, width = 128, height = 96} --DOC_HIDE


function awful.spawn(_, args) --DOC_HIDE
    local c = client.gen_fake{} --DOC_HIDE
    c:tags({args.tag}) --DOC_HIDE
    assert(#c:tags() == 1) --DOC_HIDE
    assert(c:tags()[1] == args.tag) --DOC_HIDE
end --DOC_HIDE

module.add_event("Create differing column count tags", function() --DOC_HIDE
    -- Create a tag with column count of 1 and tag with count of 2
    awful.tag.add("1 column", {
        screen   = screen[1],
        layout   = awful.layout.suit.tile,
        column_count = 1,
    })

--DOC_NEWLINE

    awful.tag.add("2 columns", {
        screen   = screen[1],
        layout   = awful.layout.suit.tile,
        column_count = 2,
    })

--DOC_NEWLINE

    awful.tag.add("3 columns", {
        screen   = screen[1],
        layout   = awful.layout.suit.tile,
        column_count = 3,
    })


end) --DOC_HIDE

module.display_tags() --DOC_HIDE

--DOC_NEWLINE

module.add_event("Add some clients", function() --DOC_HIDE
    -- Add some clients.
    for _, t in ipairs(screen[1].tags) do
        for _ = 1, 6 do
            awful.spawn("xterm", {tag = t})
        end
        assert(#t:clients() == 6) --DOC_HIDE
    end
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute {show_empty = true} --DOC_HIDE
