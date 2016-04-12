local gears_obj = require("gears.object")

local clients = {}

local client, meta = awesome._shim_fake_class()

local function add_signals(c)
    c:add_signal("property::width")
    c:add_signal("property::height")
    c:add_signal("property::x")
    c:add_signal("property::y")
    c:add_signal("property::screen")
    c:add_signal("property::geometry")
    c:add_signal("request::geometry")
    c:add_signal("swapped")
    c:add_signal("raised")
    c:add_signal("property::_label") --Used internally
end

-- Keep an history of the geometry for validation and images
local function push_geometry(c)
    table.insert(c._old_geo, c:geometry())
end

-- Undo the last move, but keep it in history
-- local function pop_geometry(c)
--     CURRENTLY UNUSED
-- end

-- Create fake clients to move around
function client.gen_fake(args)
    local ret = gears_obj()
    ret.type = "normal"
    ret.valid = true
    ret.size_hints = {}
    ret.border_width = 1

    -- Apply all properties
    for k,v in pairs(args or {}) do
        ret[k] = v
    end

    -- Tests should always set a geometry, but just in case
    for _, v in ipairs{"x","y","width","height"} do
        ret[v] = ret[v] or 1
    end


    add_signals(ret)

    -- Emulate capi.client.geometry
    function ret:geometry(new)
        if new then
            for k,v in pairs(new) do
                ret[k] = v
                ret:emit_signal("property::"..k, v)
            end
            ret:emit_signal("property::geometry", ret:geometry())
            push_geometry(ret)
        end

        return {
            x      = ret.x,
            y      = ret.y,
            width  = ret.width,
            height = ret.height,
            label  = ret._label,
        }
    end

    function ret:isvisible()
        return true
    end

    -- Used for screenshots
    function ret:set_label(text)
        ret._old_geo[#ret._old_geo]._label = text
    end

    -- Used for screenshots, hide the current client position
    function ret:_hide()
        ret._old_geo[#ret._old_geo]._hide = true
    end

    function ret:get_xproperty()
        return nil
    end

    function ret:tags(new) --FIXME
        if new then
            ret._tags = new
        end

        if ret._tags then
            return ret._tags
        end

        for _, t in ipairs(root._tags) do
            if t.screen == ret.screen then
                return {t}
            end
        end

        return {}
    end

    -- Record the geometry
    ret._old_geo = {}
    push_geometry(ret)

    -- Set the attributes
    ret.screen = args.screen or screen[1]

    -- Add to the client list
    table.insert(clients, ret)

    client.focus = ret

    client.emit_signal("manage", ret)
    assert(not args.screen or (args.screen == ret.screen))

    return setmetatable(ret, {
        __index     = function(...) return meta.__index(...) end,
        __newindex = function(...) return meta.__newindex(...) end
    })
end

function client.get(s)
    if not s then return clients end

    local s2 = screen[s]

    local ret = {}

    for _,c in ipairs(clients) do
        if c.screen == s2 then
            table.insert(ret, c)
        end
    end

    return ret
end

client:_add_signal("manage")
client:_add_signal("unmanage")
client:_add_signal("property::urgent")
client:_add_signal("untagged")
client:_add_signal("tagged")
client:_add_signal("property::shape_client_bounding")
client:_add_signal("property::shape_client_clip")
client:_add_signal("property::width")
client:_add_signal("property::height")
client:_add_signal("property::x")
client:_add_signal("property::y")
client:_add_signal("property::geometry")
client:_add_signal("focus")
client:_add_signal("new")
client:_add_signal("property::size_hints_honor")
client:_add_signal("property::struts")
client:_add_signal("property::minimized")
client:_add_signal("property::maximized_horizontal")
client:_add_signal("property::maximized_vertical")
client:_add_signal("property::sticky")
client:_add_signal("property::fullscreen")
client:_add_signal("property::border_width")
client:_add_signal("property::hidden")
client:_add_signal("property::screen")
client:_add_signal("raised")
client:_add_signal("lowered")
client:_add_signal("list")

return client

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
