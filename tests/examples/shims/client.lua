local gears_obj = require("gears.object")

local clients = {}

local client, meta = awesome._shim_fake_class()

-- Keep an history of the geometry for validation and images
local function push_geometry(c)
    table.insert(c._old_geo, c:geometry())
end

-- Undo the last move, but keep it in history
-- local function pop_geometry(c)
--     CURRENTLY UNUSED
-- end

local function titlebar_meta(c)
    for _, position in ipairs {"top", "bottom", "left", "right" } do
        c["titlebar_"..position] = function(size) --luacheck: no unused
            return drawin{}
        end
    end
end

-- Create fake clients to move around
function client.gen_fake(args)
    local ret = gears_obj()
    awesome._forward_class(ret, client)

    ret.data = {}
    ret.type = "normal"
    ret.valid = true
    ret.size_hints = {}
    ret.border_width = 1
    ret.icon_sizes = {{16,16}}
    ret.name = "Example Client"
    ret.data._struts = { top = 0, right = 0, left = 0, bottom = 0 }

    -- This is a hack because there's a `:is_transient_for(c2)` method
    -- and a `transient_for` property. It will cause a stack overflow
    -- since the auto-alias will kick in if the property is allowed to
    -- be `nil`.
    ret.transient_for = false

    -- Apply all properties
    for k,v in pairs(args or {}) do
        ret[k] = v
    end

    -- Tests should always set a geometry, but just in case
    for _, v in ipairs{"x","y","width","height"} do
        ret[v] = ret[v] or 1
    end

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

    function ret:get_icon(_)
        return require("beautiful").awesome_icon
    end

    function ret:raise()
        --TODO
    end

    function ret:lower()
        --TODO
    end

    function ret:kill()
        local old_tags = ret:tags() or {}

        for k, c in ipairs(clients) do
            if c == ret then
                ret:emit_signal("unmanaged", c)
                ret.valid = false
                table.remove(clients, k)
                break
            end
        end

        ret._tags = {}

        for _, t in ipairs(old_tags) do
            ret:emit_signal("untagged", t)
            t:emit_signal("property::tags")
        end

        assert(not ret.valid)
    end
    titlebar_meta(ret)

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

    function ret:struts(new)
        for k, v in pairs(new or {}) do
            ret.data._struts[k] = v
        end

        return ret.data._struts
    end

    function ret:struts(new)
        for k, v in pairs(new or {}) do
            ret.data._struts[k] = v
        end

        return ret.data._struts
    end

    -- Record the geometry
    ret._old_geo = {}
    push_geometry(ret)
    ret._old_geo[1]._hide = args.hide_first

    -- Set the attributes
    ret.screen = args.screen or screen[1]

    -- Good enough for the geometry and border
    ret.drawin = ret
    ret.drawable = ret

    -- Make sure the layer properties are not `nil`
    ret.ontop = false
    ret.below = false
    ret.above = false
    ret.sticky = false
    ret.maximized = false
    ret.fullscreen = false
    ret.maximized_vertical = false
    ret.maximized_horizontal = false

    -- Add to the client list
    table.insert(clients, ret)

    client.focus = ret

    setmetatable(ret, {
        __index     = function(...) return meta.__index(...) end,
        __newindex = function(...) return meta.__newindex(...) end
    })

    client.emit_signal("manage", ret)
    assert(not args.screen or (args.screen == ret.screen))

    return ret
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

return client

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
