---------------------------------------------------------------------------
--- Screen module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.screen
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

local function get_screen(s)
    return s and capi.screen[s]
end

-- we use require("awful.client") inside functions to prevent circular dependencies.
local client

local screen = {}

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

-- @param s Screen
-- @param x X coordinate of point
-- @param y Y coordinate of point
-- @return The squared distance of the screen to the provided point
function screen.getdistance_sq(s, x, y)
    s = get_screen(s)
    local geom = s.geometry
    local dist_x, dist_y = 0, 0
    if x < geom.x then
        dist_x = geom.x - x
    elseif x >= geom.x + geom.width then
        dist_x = x - geom.x - geom.width
    end
    if y < geom.y then
        dist_y = geom.y - y
    elseif y >= geom.y + geom.height then
        dist_y = y - geom.y - geom.height
    end
    return dist_x * dist_x + dist_y * dist_y
end

---
-- Return screen number corresponding to the given (pixel) coordinates.
-- The number returned can be used as an index into the global
-- `screen` table/object.
-- @param x The x coordinate
-- @param y The y coordinate
function screen.getbycoord(x, y)
    local s = capi.screen[1]
    local dist = screen.getdistance_sq(s, x, y)
    for i in capi.screen do
        local d = screen.getdistance_sq(i, x, y)
        if d < dist then
            s, dist = capi.screen[i], d
        end
    end
    return s.index
end

--- Give the focus to a screen, and move pointer to last known position on this
-- screen, or keep position relative to the current focused screen
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
-- @param dir The direction, can be either "up", "down", "left" or "right".
-- @param _screen Screen.
function screen.focus_bydirection(dir, _screen)
    local sel = get_screen(_screen or screen.focused())
    if sel then
        local geomtbl = {}
        for s in capi.screen do
            geomtbl[s] = capi.screen[s].geometry
        end
        local target = util.get_rectangle_in_direction(dir, geomtbl, sel.geometry)
        if target then
            return screen.focus(target)
        end
    end
end

--- Give the focus to a screen, and move pointer to last known position on this
-- screen, or keep position relative to the current focused screen
-- @param i Value to add to the current focused screen index. 1 will focus next
-- screen, -1 would focus the previous one.
function screen.focus_relative(i)
    return screen.focus(util.cycle(capi.screen.count(), screen.focused().index + i))
end

--- Get or set the screen padding.
-- @param _screen The screen object to change the padding on
-- @param[opt=nil] padding The padding, a table with 'top', 'left', 'right' and/or
-- 'bottom' or a number value to apply set the same padding on all sides. Can be
--  nil if you only want to retrieve padding
-- @treturn table A table with left, right, top and bottom number values.
function screen.padding(_screen, padding)
    if type(padding) == "number" then
        padding = {
            left   = padding,
            right  = padding,
            top    = padding,
            bottom = padding,
        }
    end

    _screen = get_screen(_screen)
    if padding then
        data.padding[_screen] = padding
        _screen:emit_signal("padding")
    end

    local p = data.padding[_screen] or {}

    -- Create a copy to avoid accidental mutation and nil values
    return {
        left   = p.left   or 0,
        right  = p.right  or 0,
        top    = p.top    or 0,
        bottom = p.bottom or 0,
    }
end

--- Get the focused screen.
--
-- Valid arguments are:
--
-- * **client**: Use the client screen instead of the mouse screen.
-- * **mouse**: Use the mouse screen (default).
--
-- It is possible to set `awful.screen.default_focused_args` to override the
-- default settings.
--
-- @tparam[opt={}] table args Some arguments.
-- @treturn screen The focused screen object
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
-- @tparam[opt=mouse.screen] screen s A screen
-- @tparam[opt={}] table args The arguments
-- @treturn table A table with *x*, *y*, *width* and *height*.
function screen.get_bounding_geometry(s, args)
    args = args or {}

    -- If the tag has a geometry, assume it is right
    if args.tag then
        s = args.tag.screen
    end

    s = get_screen(s or capi.mouse.screen)

    local geo = args.bounding_rect or (args.parent and args.parent:geometry()) or
        s[args.honor_workarea and "workarea" or "geometry"]

    if (not args.parent) and (not args.bounding_rect) and args.honor_padding then
        local padding = screen.padding(s)
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

capi.screen.add_signal("padding")

-- Extend the luaobject
object.properties(capi.screen, {auto_emit=true})

return screen

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
