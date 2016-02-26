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

local function get_screen(s)
    return s and screen[s]
end

-- we use require("awful.client") inside functions to prevent circular dependencies.
local client

local screen = {}

local data = {}
data.padding = {}

screen.mouse_per_screen = {}

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
    local s = screen[1]
    local dist = screen.getdistance_sq(s, x, y)
    for i = 2, capi.screen:count() do
        local d = screen.getdistance_sq(i, x, y)
        if d < dist then
            s, dist = screen[i], d
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

    local c = client.focus.history.get(_screen.index, 0)
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
        for s = 1, capi.screen.count() do
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
    return screen.focus(util.cycle(capi.screen.count(), screen.focused() + i))
end

--- Get or set the screen padding.
-- @param _screen The screen object to change the padding on
-- @param padding The padding, an table with 'top', 'left', 'right' and/or
-- 'bottom'. Can be nil if you only want to retrieve padding
function screen.padding(_screen, padding)
    _screen = get_screen(_screen)
    if padding then
        data.padding[_screen] = padding
        _screen:emit_signal("padding")
    end
    return data.padding[_screen]
end

--- Get the focused screen.
-- This can be replaced in a user's config.
-- @treturn integer
function screen.focused()
    return capi.mouse.screen
end

capi.screen.add_signal("padding")

return screen

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
