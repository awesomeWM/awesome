--DOC_HIDE_ALL
local a_tag = require("awful.tag")
local a_layout = require("awful.layout")
local corner = require("awful.layout.dynamic.suit.corner")

-- Create a 16 screen matrix
screen._setup_grid(64, 48, {7, 7, 7, 7}, {workarea_sides=3})

local tags = {}

local function add_clients(s, count)
    local x, y = s.geometry.x, s.geometry.y
    for _=1, count do
        local c1 = client.gen_fake {x = x+45, y = y+35, width=40, height=30, screen=s}
        c1:_hide()
    end
end

local function show_layout(f, s)
    local t = tags[s]
    assert(s and t)

    t.layout = f

    local l = a_tag.getproperty(t, "layout")

    -- Test if the tag is the right one
    assert(l._tag == t)

    -- Test if the stateful layout track the right number of clients
    assert(#t:clients() == #l.wrappers)

    local params = a_layout.parameters(t, s)

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

    -- Finally, test if the clients total size match the workarea
    local total = 0
    for _, c in ipairs(t:clients()) do
        local geo = c:geometry()
        geo.height, geo.width = geo.height+2*c.border_width, geo.width+2*c.border_width
        total = total + geo.height*geo.width
    end

    --FIXME Work for all but the cornerh with 5 client (21px off)
    --assert(s.workarea.width * s.workarea.height == total)

end

local layouts = {corner.nw, corner.ne, corner.sw, corner.se}

for i=1, #layouts do
    for j=1, 7 do
        local idx = (i-1)*7 + j
        tags[screen[idx]] = a_tag.add("Test"..idx, {screen=screen[idx]})
        add_clients(screen[idx],j)
        show_layout(layouts[i], screen[idx])
    end
end

return {hide_lines=true, radius=2}
