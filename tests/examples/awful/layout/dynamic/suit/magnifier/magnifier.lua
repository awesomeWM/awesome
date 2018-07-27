--DOC_HIDE_ALL
local a_tag     = require("awful.tag")
local a_layout  = require("awful.layout")
local magnifier = require("awful.layout.dynamic.suit.magnifier")

local function add_clients(s)
    local x, y = s.geometry.x, s.geometry.y
    for _=1, 5 do
        client.gen_fake{x = x+45, y = y+35, width=40, height=30, screen=s}:_hide()
    end
end

-- Add some tags and clients
local tags = {}
tags[screen[1]] = a_tag.add("Test1", {screen=screen[1]})
add_clients(screen[1])

local function show_layout(f, s, name)
    local t = tags[s]

    -- Test if the layout exist
    assert(f)

    t.layout = f

    local l = a_tag.getproperty(t, "layout")

    -- Test if the right type of layout was created
    assert(l.name == name)

    -- Test is setlayout did create a stateful layout
    assert(l.is_dynamic)

    -- Test if the stateful layout track the right number of clients
    assert(#t:clients() == #l.wrappers)

    -- Cheat a little to avoid having to enable the event loop on this test
--     l.widget:before_draw_children(
--         nil, nil, s.workarea.width, s.workarea.height
--     )

    local params = a_layout.parameters(t, s)

    -- Test if the tag state is correct
    assert(l.active)

    -- Test if the widget has been created
    assert(l.widget)

    -- Test if the number of clients wrapper is right and if they are there
    -- only once
    local all_widget = l.widget:get_all_children()
    local cls, inv = {},{}
    for _, w in ipairs(all_widget) do
        if w._client then
            assert(not inv[w._client])
            inv[w._client] = w
            table.insert(cls, w._client)
        end
    end
    assert(#cls == #t:clients())

    l.arrange(params)

end

show_layout(magnifier, screen[1], "magnifier")

return {hide_lines=true}
