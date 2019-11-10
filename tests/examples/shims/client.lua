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

local properties = {}

-- Emit the request::geometry signal to make immobilized and awful.ewmh work.
for _, prop in ipairs {
  "maximized", "maximized_horizontal", "maximized_vertical", "fullscreen" } do
    properties["get_"..prop] = function(self)
        return self._private[prop] or false
    end

    properties["set_"..prop] = function(self, value)
        self._private[prop] = value or false

        if value then
            self:emit_signal("request::geometry", prop, nil)
        end

        self:emit_signal("property::"..prop, value)
    end
end

function properties.get_screen(self)
    return self._private.screen or screen[1]
end

function properties.set_screen(self, s)
    s = screen[s]
    self._private.screen = s

    if self.x < s.geometry.x or self.x > s.geometry.x+s.geometry.width then
        self:geometry { x = s.geometry.x }
    end

    if self.y < s.geometry.y or self.y > s.geometry.y+s.geometry.height then
        self:geometry { y = s.geometry.y }
    end

    self:emit_signal("property::screen")
end

-- Create fake clients to move around
function client.gen_fake(args)
    local ret = gears_obj()
    awesome._forward_class(ret, client)

    ret._private = {}
    ret.type = "normal"
    ret.valid = true
    ret.size_hints = {}
    ret.border_width = 1
    ret.icon_sizes = {{16,16}}
    ret.name = "Example Client"
    ret._private._struts = { top = 0, right = 0, left = 0, bottom = 0 }

    --TODO v5: remove this. This was a private API and thus doesn't need to be
    -- officially deprecated.
    ret.data = ret._private

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
        assert((not args[v]) or ret[v] == args[v])
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

    function ret:set_xproperty(prop, value)
        ret[prop] = value
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

    function ret:apply_size_hints(w, h)
        return w or ret.width, h or ret.height
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
            ret._private._struts[k] = v
        end

        return ret._private._struts
    end

    -- Set a dummy one for now since set_screen will corrupt it.
    ret._old_geo = {}

    -- Set the screen attributes.
    local s = args.screen

    -- Pick the screen from the geometry.
    if not s then
        for s2 in screen do
            local geo = s2.geometry
            if geo.x >= ret.x and geo.y >= ret.y and ret.x < geo.x+geo.width
              and ret.y < geo.y+geo.height then
                s = s2
            end
        end
    end

    -- This will happen if the screen coords are not {0,0} and the client had
    -- an unspecified position.
    s = s or screen[1]

    properties.set_screen(ret, s)

    -- Set the geometry *again* because the screen possibly overwrote it.
    for _, v in ipairs{"x","y","width","height"} do
        ret[v] = args[v] or ret[v]
    end

    -- Record the geometry
    ret._old_geo = {}
    push_geometry(ret)
    ret._old_geo[1]._hide = args.hide_first

    -- Good enough for the geometry and border
    ret.drawin = ret
    ret.drawable = ret

    -- Make sure the layer properties are not `nil`
    ret.ontop = false
    ret.below = false
    ret.above = false
    ret.sticky = false

    -- Declare the deprecated buttons and keys methods.
    function ret:_keys(new)
        if new then
            ret._private.keys = new
        end

        return ret._private.keys or {}
    end

    function ret:_buttons(new)
        if new then
            ret._private.buttons = new
        end

        return ret._private.buttons or {}
    end

    -- Add to the client list
    table.insert(clients, ret)

    client.focus = ret

    setmetatable(ret, {
        __index     = function(self, key)
            if properties["get_"..key] then
                return properties["get_"..key](self)
            end

            return meta.__index(self, key)
        end,
        __newindex = function(self, key, value)

            if properties["set_"..key] then
                return properties["set_"..key](self, value)
            end

            return meta.__newindex(self, key, value)
        end
    })

    client.emit_signal("request::manage", ret)

    --TODO v6 remove this.
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
