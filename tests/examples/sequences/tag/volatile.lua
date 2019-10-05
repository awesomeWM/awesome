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

module.add_event("Create a non-volatile and a volatile tag", function() --DOC_HIDE
    -- Create a non-volatile and a volatile tag.
    awful.tag.add("Non-Volatile", {
        screen   = screen[1],
        layout   = awful.layout.suit.corner.nw,
        volatile = false,
    })

--DOC_NEWLINE

    awful.tag.add("Volatile", {
        screen   = screen[1],
        layout   = awful.layout.suit.corner.nw,
        volatile = true,
    })

end) --DOC_HIDE

module.display_tags() --DOC_HIDE

--DOC_NEWLINE

module.add_event("Add some clients", function() --DOC_HIDE
    -- Add some clients.
    for _, t in ipairs(screen[1].tags) do
        for _ = 1, 5 do
            awful.spawn("xterm", {tag = t})
        end
        assert(#t:clients() == 5) --DOC_HIDE
    end
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

--DOC_NEWLINE

module.add_event("Kill all clients", function() --DOC_HIDE
    -- Kill all clients.
    while #client.get() ~= 0 do
        client.get()[1]:kill()
    end
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute {show_empty = true} --DOC_HIDE
