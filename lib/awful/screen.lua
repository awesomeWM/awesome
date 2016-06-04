---------------------------------------------------------------------------
--- Screen module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module screen
---------------------------------------------------------------------------

-- Grab environment we need
local capi =
{
    mouse = mouse,
    screen = screen,
    client = client
}
local util = require("awful.util")
local object = require("gears.object")
local grect =  require("gears.geometry").rectangle

local function get_screen(s)
    return s and capi.screen[s]
end

-- we use require("awful.client") inside functions to prevent circular dependencies.
local client

local screen = {object={}}

local data = {}
data.padding = {}

screen.mouse_per_screen = {}

--- Take an input geometry and substract/add a delta
-- @tparam table geo A geometry (width, height, x, y) table
-- @tparam table delta A delata table (top, bottom, x, y)
-- @treturn table A geometry (width, height, x, y) table
local function apply_geometry_ajustments(geo, delta)
    return {
        x      = geo.x      + (delta.left or 0),
        y      = geo.y      + (delta.top  or 0),
        width  = geo.width  - (delta.left or 0) - (delta.right  or 0),
        height = geo.height - (delta.top  or 0) - (delta.bottom or 0),
    }
end

--- Get the square distance between a `screen` and a point
-- @deprecated awful.screen.getdistance_sq
-- @param s Screen
-- @param x X coordinate of point
-- @param y Y coordinate of point
-- @return The squared distance of the screen to the provided point
-- @see screen.get_square_distance
function screen.getdistance_sq(s, x, y)
    util.deprecate("Use s:get_square_distance(x, y) instead of awful.screen.getdistance_sq")
    return screen.object.get_square_distance(s, x, y)
end

--- Get the square distance between a `screen` and a point
-- @function screen.get_square_distance
-- @tparam number x X coordinate of point
-- @tparam number y Y coordinate of point
-- @treturn number The squared distance of the screen to the provided point
function screen.object.get_square_distance(self, x, y)
    return grect.get_square_distance(get_screen(self).geometry, x, y)
end

---
-- Return screen number corresponding to the given (pixel) coordinates.
-- The number returned can be used as an index into the global
-- `screen` table/object.
-- @function awful.screen.getbycoord
-- @tparam number x The x coordinate
-- @tparam number y The y coordinate
-- @treturn ?number The screen index
function screen.getbycoord(x, y)
    local s, sgeos = capi.screen.primary, {}

    for scr in capi.screen do
        sgeos[scr] = scr.geometry
    end

    s = grect.get_closest_by_coord(sgeos, x, y) or s

    return s and s.index
end

--- Give the focus to a screen, and move pointer to last known position on this
-- screen, or keep position relative to the current focused screen
-- @function awful.screen.focus
-- @param _screen Screen number (defaults / falls back to mouse.screen).
function screen.focus(_screen)
    client = client or require("awful.client")
    if type(_screen) == "number" and _screen > capi.screen.count() then _screen = screen.focused() end
    _screen = get_screen(_screen)

    -- screen and pos for current screen
    local s = get_screen(capi.mouse.screen)
    local pos

    if not screen.mouse_per_screen[_screen] then
        -- This is the first time we enter this screen,
        -- keep relative mouse position on the new screen
        pos = capi.mouse.coords()
        local relx = (pos.x - s.geometry.x) / s.geometry.width
        local rely = (pos.y - s.geometry.y) / s.geometry.height

        pos.x = _screen.geometry.x + relx * _screen.geometry.width
        pos.y = _screen.geometry.y + rely * _screen.geometry.height
    else
        -- restore mouse position
        pos = screen.mouse_per_screen[_screen]
    end

    -- save pointer position of current screen
    screen.mouse_per_screen[s] = capi.mouse.coords()

   -- move cursor without triggering signals mouse::enter and mouse::leave
    capi.mouse.coords(pos, true)

    local c = client.focus.history.get(_screen, 0)
    if c then
        c:emit_signal("request::activate", "screen.focus", {raise=false})
    end
end

--- Give the focus to a screen, and move pointer to last known position on this
-- screen, or keep position relative to the current focused screen
-- @function awful.screen.focus_bydirection
-- @param dir The direction, can be either "up", "down", "left" or "right".
-- @param _screen Screen.
function screen.focus_bydirection(dir, _screen)
    local sel = get_screen(_screen or screen.focused())
    if sel then
        local geomtbl = {}
        for s in capi.screen do
            geomtbl[s] = capi.screen[s].geometry
        end
        local target = grect.get_in_direction(dir, geomtbl, sel.geometry)
        if target then
            return screen.focus(target)
        end
    end
end

--- Give the focus to a screen, and move pointer to last known position on this
-- screen, or keep position relative to the current focused screen
-- @function awful.screen.focus_relative
-- @param i Value to add to the current focused screen index. 1 will focus next
-- screen, -1 would focus the previous one.
function screen.focus_relative(i)
    return screen.focus(util.cycle(capi.screen.count(), screen.focused().index + i))
end

--- Get or set the screen padding.
-- @deprecated awful.screen.padding
-- @param _screen The screen object to change the padding on
-- @param[opt=nil] padding The padding, a table with 'top', 'left', 'right' and/or
-- 'bottom' or a number value to apply set the same padding on all sides. Can be
--  nil if you only want to retrieve padding
-- @treturn table A table with left, right, top and bottom number values.
-- @see padding
function screen.padding(_screen, padding)
    util.deprecate("Use _screen.padding = value instead of awful.screen.padding")
    if padding then
        screen.object.set_padding(_screen, padding)
    end
    return screen.object.get_padding(_screen)
end

--- The screen padding.
-- Add a "buffer" section on each side of the screen where the tiled client
-- wont be.
--
-- **Signal:**
--
-- * *property::padding*
--
-- @property padding
-- @param table
-- @tfield integer table.x The horizontal position
-- @tfield integer table.y The vertical position
-- @tfield integer table.width The width
-- @tfield integer table.height The height

function screen.object.get_padding(self)
    local p = data.padding[self] or {}

    -- Create a copy to avoid accidental mutation and nil values
    return {
        left   = p.left   or 0,
        right  = p.right  or 0,
        top    = p.top    or 0,
        bottom = p.bottom or 0,
    }
end

function screen.object.set_padding(self, padding)
    if type(padding) == "number" then
        padding = {
            left   = padding,
            right  = padding,
            top    = padding,
            bottom = padding,
        }
    end

    self = get_screen(self)
    if padding then
        data.padding[self] = padding
        self:emit_signal("padding")
    end
end

--- The defaults arguments for `awful.screen.focused`
-- @tfield[opt=nil] table awful.screen.default_focused_args

--- Get the focused screen.
--
-- It is possible to set `awful.screen.default_focused_args` to override the
-- default settings.
--
-- @function awful.screen.focused
-- @tparam[opt] table args
-- @tparam[opt=false] boolean args.client Use the client screen instead of the
--   mouse screen.
-- @tparam[opt=true] boolean args.mouse Use the mouse screen
-- @treturn ?screen The focused screen object, or `nil` in case no screen is
--   present currently.
function screen.focused(args)
    args = args or screen.default_focused_args or {}
    return get_screen(args.client and capi.client.screen or capi.mouse.screen)
end

--- Get a placement bounding geometry.
-- This method compute different variants of the "usable" screen geometry.
--
-- Valid arguments are:
--
-- * **honor_padding**: Honor the screen padding.
-- * **honor_workarea**: Honor the screen workarea
-- * **margins**: Apply some margins on the output. This can wither be a number
--    or a table with *left*, *right*, *top* and *bottom* keys.
-- * **tag**: Use the tag screen instead of `s`
-- * **parent**: A parent drawable to use a base geometry
-- * **bounding_rect**: A bounding rectangle. This parameter is incompatible with
--    `honor_workarea`.
--
-- @function screen.get_bounding_geometry
-- @tparam[opt={}] table args The arguments
-- @treturn table A table with *x*, *y*, *width* and *height*.
-- @usage local geo = screen:get_bounding_geometry {
--     honor_padding  = true,
--     honor_workarea = true,
--     margins        = {
--          left = 20,
--     },
-- }
function screen.object.get_bounding_geometry(self, args)
    args = args or {}

    -- If the tag has a geometry, assume it is right
    if args.tag then
        self = args.tag.screen
    end

    self = get_screen(self or capi.mouse.screen)

    local geo = args.bounding_rect or (args.parent and args.parent:geometry()) or
        self[args.honor_workarea and "workarea" or "geometry"]

    if (not args.parent) and (not args.bounding_rect) and args.honor_padding then
        local padding = self.padding
        geo = apply_geometry_ajustments(geo, padding)
    end

    if args.margins then
        geo = apply_geometry_ajustments(geo,
            type(args.margins) == "table" and args.margins or {
                left = args.margins, right  = args.margins,
                top  = args.margins, bottom = args.margins,
            }
        )
    end

    return geo
end

--- Get the list of the screen visible clients.
--
-- Minimized and unmanaged clients are not included in this list as they are
-- technically not on the screen.
--
-- The clients on tags currently not visible are not part of this list.
--
-- @property clients
-- @param table The clients list, ordered top to bottom
-- @see all_clients
-- @see hidden_clients
-- @see client.get

function screen.object.get_clients(s)
    local cls = capi.client.get(s, true)
    local vcls = {}
    for _, c in pairs(cls) do
        if c:isvisible() then
            table.insert(vcls, c)
        end
    end
    return vcls
end

--- Get the list of the clients assigned to the screen but not currently
-- visible.
--
-- This include minimized clients and clients on hidden tags.
--
-- @property hidden_clients
-- @param table The clients list, ordered top to bottom
-- @see clients
-- @see all_clients
-- @see client.get

function screen.object.get_hidden_clients(s)
    local cls = capi.client.get(s, true)
    local vcls = {}
    for _, c in pairs(cls) do
        if not c:isvisible() then
            table.insert(vcls, c)
        end
    end
    return vcls
end

--- Get all clients assigned to the screen.
--
-- @property all_clients
-- @param table The clients list, ordered top to bottom
-- @see clients
-- @see hidden_clients
-- @see client.get

function screen.object.get_all_clients(s)
    return capi.client.get(s, true)
end

--- Get the list of the screen tiled clients.
--
-- Same as s.clients, but excluding:
--
-- * fullscreen clients
-- * maximized clients
-- * floating clients
--
-- @property tiled_clients
-- @param table The clients list, ordered top to bottom

function screen.object.get_tiled_clients(s)
    local clients = s.clients
    local tclients = {}
    -- Remove floating clients
    for _, c in pairs(clients) do
        if not c.floating
            and not c.fullscreen
            and not c.maximized_vertical
            and not c.maximized_horizontal then
            table.insert(tclients, c)
        end
    end
    return tclients
end

--- Call a function for each existing and created-in-the-future screen.
-- @function awful.screen.connect_for_each_screen
-- @tparam function func The function to call.
-- @tparam screen func.screen The screen
function screen.connect_for_each_screen(func)
    for s in capi.screen do
        func(s)
    end
    capi.screen.connect_signal("added", func)
end

--- Undo the effect of connect_for_each_screen.
-- @function awful.screen.disconnect_for_each_screen
-- @tparam function func The function that should no longer be called.
function screen.disconnect_for_each_screen(func)
    capi.screen.disconnect_signal("added", func)
end

--- A list of all tags on the screen.
--
-- This property is read only, use `tag.screen`, `awful.tag.add`, `awful.tag.new`
-- or `t:delete()` to alter this list.
--
-- @property tags
-- @param table
-- @treturn table A table with all available tags

function screen.object.get_tags(s, unordered)
    local tags = {}

    for _, t in ipairs(root.tags()) do
        if get_screen(t.screen) == s then
            table.insert(tags, t)
        end
    end

    -- Avoid infinite loop, + save some time
    if not unordered then
        table.sort(tags, function(a, b)
            return (a.index or 9999) < (b.index or 9999)
        end)
    end

    return tags
end

--- A list of all selected tags on the screen.
-- @property selected_tags
-- @param table
-- @treturn table A table with all selected tags.
-- @see tag.selected
-- @see client.to_selected_tags

function screen.object.get_selected_tags(s)
    local tags = screen.object.get_tags(s, true)

    local vtags = {}
    for _, t in pairs(tags) do
        if t.selected then
            vtags[#vtags + 1] = t
        end
    end
    return vtags
end

--- The first selected tag.
-- @property selected_tag
-- @param table
-- @treturn ?tag The first selected tag or nil
-- @see tag.selected
-- @see selected_tags

function screen.object.get_selected_tag(s)
    return screen.object.get_selected_tags(s)[1]
end


--- When the tag history changed.
-- @signal tag::history::update

-- Extend the luaobject
object.properties(capi.screen, {
    getter_class = screen.object,
    setter_class = screen.object,
    auto_emit    = true,
})

return screen

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
