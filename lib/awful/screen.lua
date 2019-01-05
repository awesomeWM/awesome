---------------------------------------------------------------------------
--- Screen module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module screen
---------------------------------------------------------------------------

-- Grab environment we need
local capi =
{
    mouse = mouse,
    screen = screen,
    client = client,
    awesome = awesome,
}
local gdebug = require("gears.debug")
local gmath = require("gears.math")
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

--- Take an input geometry and substract/add a delta.
-- @tparam table geo A geometry (width, height, x, y) table.
-- @tparam table delta A delta table (top, bottom, x, y).
-- @treturn table A geometry (width, height, x, y) table.
local function apply_geometry_ajustments(geo, delta)
    return {
        x      = geo.x      + (delta.left or 0),
        y      = geo.y      + (delta.top  or 0),
        width  = geo.width  - (delta.left or 0) - (delta.right  or 0),
        height = geo.height - (delta.top  or 0) - (delta.bottom or 0),
    }
end

--- Get the square distance between a `screen` and a point.
-- @deprecated awful.screen.getdistance_sq
-- @param s Screen
-- @param x X coordinate of point
-- @param y Y coordinate of point
-- @return The squared distance of the screen to the provided point.
-- @see screen.get_square_distance
function screen.getdistance_sq(s, x, y)
    gdebug.deprecate("Use s:get_square_distance(x, y) instead of awful.screen.getdistance_sq", {deprecated_in=4})
    return screen.object.get_square_distance(s, x, y)
end

--- Get the square distance between a `screen` and a point.
-- @function screen.get_square_distance
-- @tparam number x X coordinate of point
-- @tparam number y Y coordinate of point
-- @treturn number The squared distance of the screen to the provided point.
function screen.object.get_square_distance(self, x, y)
    return grect.get_square_distance(get_screen(self).geometry, x, y)
end

--- Return the screen index corresponding to the given (pixel) coordinates.
--
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

--- Move the focus to a screen.
--
-- This moves the mouse pointer to the last known position on the new screen,
-- or keeps its position relative to the current focused screen.
-- @function awful.screen.focus
-- @screen _screen Screen number (defaults / falls back to mouse.screen).
function screen.focus(_screen)
    client = client or require("awful.client")
    if type(_screen) == "number" and _screen > capi.screen.count() then _screen = screen.focused() end
    _screen = get_screen(_screen)

    -- screen and pos for current screen
    local s = get_screen(capi.mouse.screen)
    local pos

    if not _screen.mouse_per_screen then
        -- This is the first time we enter this screen,
        -- keep relative mouse position on the new screen.
        pos = capi.mouse.coords()
        local relx = (pos.x - s.geometry.x) / s.geometry.width
        local rely = (pos.y - s.geometry.y) / s.geometry.height

        pos.x = _screen.geometry.x + relx * _screen.geometry.width
        pos.y = _screen.geometry.y + rely * _screen.geometry.height
    else
        -- restore mouse position
        pos = _screen.mouse_per_screen
    end

    -- save pointer position of current screen
    s.mouse_per_screen = capi.mouse.coords()

   -- move cursor without triggering signals mouse::enter and mouse::leave
    capi.mouse.coords(pos, true)

    local c = client.focus.history.get(_screen, 0)
    if c then
        c:emit_signal("request::activate", "screen.focus", {raise=false})
    end
end

--- Get the next screen in a specific direction.
--
-- This gets the next screen relative to this one in
-- the specified direction.
--
-- @function screen:get_next_in_direction
-- @param self Screen.
-- @param dir The direction, can be either "up", "down", "left" or "right".
function screen.object.get_next_in_direction(self, dir)
    local sel = get_screen(self)
    if not sel then
        return
    end

    local geomtbl = {}
    for s in capi.screen do
        geomtbl[s] = s.geometry
    end

    return grect.get_in_direction(dir, geomtbl, sel.geometry)
end

--- Move the focus to a screen in a specific direction.
--
-- This moves the mouse pointer to the last known position on the new screen,
-- or keeps its position relative to the current focused screen.
-- @function awful.screen.focus_bydirection
-- @param dir The direction, can be either "up", "down", "left" or "right".
-- @param _screen Screen.
function screen.focus_bydirection(dir, _screen)
    local sel = get_screen(_screen or screen.focused())
    local target = sel:get_next_in_direction(dir)

    if target then
        return screen.focus(target)
    end
end

--- Move the focus to a screen relative to the current one,
--
-- This moves the mouse pointer to the last known position on the new screen,
-- or keeps its position relative to the current focused screen.
--
-- @function awful.screen.focus_relative
-- @tparam int offset Value to add to the current focused screen index. 1 to
--   focus the next one, -1 to focus the previous one.
function screen.focus_relative(offset)
    return screen.focus(gmath.cycle(capi.screen.count(),
                                   screen.focused().index + offset))
end

--- Get or set the screen padding.
--
-- @deprecated awful.screen.padding
-- @param _screen The screen object to change the padding on
-- @param[opt=nil] padding The padding, a table with 'top', 'left', 'right' and/or
-- 'bottom' or a number value to apply set the same padding on all sides. Can be
--  nil if you only want to retrieve padding
-- @treturn table A table with left, right, top and bottom number values.
-- @see padding
function screen.padding(_screen, padding)
    gdebug.deprecate("Use _screen.padding = value instead of awful.screen.padding", {deprecated_in=4})
    if padding then
        screen.object.set_padding(_screen, padding)
    end
    return screen.object.get_padding(_screen)
end

--- The screen padding.
--
-- This adds a "buffer" section on each side of the screen.
--
-- **Signal:**
--
-- * *property::padding*
--
-- @property padding
-- @param table
-- @tfield integer table.left The padding on the left.
-- @tfield integer table.right The padding on the right.
-- @tfield integer table.top The padding on the top.
-- @tfield integer table.bottom The padding on the bottom.

function screen.object.get_padding(self)
    local p = data.padding[self] or {}
    -- Create a copy to avoid accidental mutation and nil values.
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

--- Get the preferred screen in the context of a client.
--
-- This is exactly the same as `awful.screen.focused` except that it avoids
-- clients being moved when Awesome is restarted.
-- This is used in the default `rc.lua` to ensure clients get assigned to the
-- focused screen by default.
-- @tparam client c A client.
-- @treturn screen The preferred screen.
function screen.preferred(c)
    return capi.awesome.startup and c.screen or screen.focused()
end

--- The defaults arguments for `awful.screen.focused`.
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
    return get_screen(
        args.client and capi.client.focus and capi.client.focus.screen or capi.mouse.screen
    )
end

--- Get a placement bounding geometry.
--
-- This method computes the different variants of the "usable" screen geometry.
--
-- @function screen.get_bounding_geometry
-- @tparam[opt={}] table args The arguments
-- @tparam[opt=false] boolean args.honor_padding Whether to honor the screen's padding.
-- @tparam[opt=false] boolean args.honor_workarea Whether to honor the screen's workarea.
-- @tparam[opt] int|table args.margins Apply some margins on the output.
--   This can either be a number or a table with *left*, *right*, *top*
--   and *bottom* keys.
-- @tag[opt] args.tag Use this tag's screen.
-- @tparam[opt] drawable args.parent A parent drawable to use as base geometry.
-- @tab[opt] args.bounding_rect A bounding rectangle. This parameter is
--   incompatible with `honor_workarea`.
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

--- The list of visible clients for the screen.
--
-- Minimized and unmanaged clients are not included in this list as they are
-- technically not on the screen.
--
-- The clients on tags that are currently not visible are not part of this list.
--
-- Clients are returned using the stacking order (from top to bottom).
-- See `get_clients` if you want them in the order used in the tasklist by
-- default.
--
-- @property clients
-- @param table The clients list, ordered from top to bottom.
-- @see all_clients
-- @see hidden_clients
-- @see client.get

--- Get the list of visible clients for the screen.
--
-- This is used by `screen.clients` internally (with `stacked=true`).
--
-- @function screen:get_clients
-- @tparam[opt=true] boolean stacked Use stacking order? (top to bottom)
-- @treturn table The clients list.
function screen.object.get_clients(s, stacked)
    local cls = capi.client.get(s, stacked == nil and true or stacked)
    local vcls = {}
    for _, c in pairs(cls) do
        if c:isvisible() then
            table.insert(vcls, c)
        end
    end
    return vcls
end

--- Get the list of clients assigned to the screen but not currently visible.
--
-- This includes minimized clients and clients on hidden tags.
--
-- @property hidden_clients
-- @param table The clients list, ordered from top to bottom.
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

--- All clients assigned to the screen.
--
-- @property all_clients
-- @param table The clients list, ordered from top to bottom.
-- @see clients
-- @see hidden_clients
-- @see client.get

--- Get all clients assigned to the screen.
--
-- This is used by `all_clients` internally (with `stacked=true`).
--
-- @function screen:get_all_clients
-- @tparam[opt=true] boolean stacked Use stacking order? (top to bottom)
-- @treturn table The clients list.
function screen.object.get_all_clients(s, stacked)
    return capi.client.get(s, stacked == nil and true or stacked)
end

--- Tiled clients for the screen.
--
-- Same as `clients`, but excluding:
--
-- * fullscreen clients
-- * maximized clients
-- * floating clients
--
-- @property tiled_clients
-- @param table The clients list, ordered from top to bottom.

--- Get tiled clients for the screen.
--
-- This is used by `tiles_clients` internally (with `stacked=true`).
--
-- @function screen:get_tiled_clients
-- @tparam[opt=true] boolean stacked Use stacking order? (top to bottom)
-- @treturn table The clients list.
function screen.object.get_tiled_clients(s, stacked)
    local clients = s:get_clients(stacked)
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
--
-- @function awful.screen.connect_for_each_screen
-- @tparam function func The function to call.
-- @screen func.screen The screen.
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
-- This property is read only, use `tag.screen`, `awful.tag.add`,
-- `awful.tag.new` or `t:delete()` to alter this list.
--
-- @property tags
-- @param table
-- @treturn table A table with all available tags.

function screen.object.get_tags(s, unordered)
    local tags = {}

    for _, t in ipairs(root.tags()) do
        if get_screen(t.screen) == s then
            table.insert(tags, t)
        end
    end

    -- Avoid infinite loop and save some time.
    if not unordered then
        table.sort(tags, function(a, b)
            return (a.index or math.huge) < (b.index or math.huge)
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
-- @param tag
-- @treturn ?tag The first selected tag or nil.
-- @see tag.selected
-- @see selected_tags

function screen.object.get_selected_tag(s)
    return screen.object.get_selected_tags(s)[1]
end

--- Enable the automatic calculation of the screen DPI (experimental).
--
-- This will cause many elements such as the font and some widgets to be scaled
-- so they look the same (physical) size on different devices with different
-- pixel density.
--
-- It is calculated using the information provided from `xrandr`.
--
-- When enabled, the theme and configuration must avoid using pixel sizes for
-- different elements as this will cause misalignment or hidden content on some
-- devices.
--
-- Note that it has to be called early in `rc.lua` and requires restarting
-- awesome to take effect. It is disabled by default and changes introduced in
-- minor releases of Awesome may slightly break the behavior as more components
-- gain support for HiDPI.
--
-- When disabled the DPI is acquired from the `Xft.dpi` X resource (xrdb),
-- defaulting to 96.
--
-- @tparam boolean enabled Enable or disable automatic DPI support.
function screen.set_auto_dpi_enabled(enabled)
    for s in capi.screen do
        s.data.dpi_cache = nil
    end
    data.autodpi = enabled
end


--- The number of pixels per inch of the screen.
-- @property dpi
-- @treturn number the DPI value.

local xft_dpi, fallback_dpi

local function get_fallback()
    local mm_per_inch = 25.4

    -- Following Keith Packard's whitepaper on Xft,
    -- https://keithp.com/~keithp/talks/xtc2001/paper/xft.html#sec-editing
    -- the proper fallback for Xft.dpi is the vertical DPI reported by
    -- the X server. This will generally be 96 on Xorg, unless the user
    -- has configured it differently
    if root and not fallback_dpi then
        local _, h = root.size()
        local _, hmm = root.size_mm()
        fallback_dpi = hmm ~= 0 and h * mm_per_inch / hmm
    end

    return fallback_dpi or 96
end

function screen.object.get_dpi(s)
    local mm_per_inch = 25.4

    if s.data.dpi or s.data.dpi_cache then
        return s.data.dpi or s.data.dpi_cache
    end

    -- Xft.dpi is explicit user configuration, so honor it
    if not xft_dpi and awesome and awesome.xrdb_get_value then
        xft_dpi = tonumber(awesome.xrdb_get_value("", "Xft.dpi")) or false
    end

    if xft_dpi then
        s.data.dpi_cache = xft_dpi
        return s.data.dpi_cache
    end

    if not data.autodpi then
        s.data.dpi_cache = get_fallback()
        return s.data.dpi_cache
    end

    -- Try to compute DPI based on outputs (use the minimum)
    local dpi = nil
    local geo = s.geometry
    for _, o in pairs(s.outputs) do
        -- Ignore outputs with width/height 0
        if o.mm_width ~= 0 and o.mm_height ~= 0 then
            local dpix = geo.width * mm_per_inch / o.mm_width
            local dpiy = geo.height * mm_per_inch / o.mm_height
            dpi = math.min(dpix, dpiy, dpi or dpix)
        end
    end
    if dpi then
        s.data.dpi_cache = dpi
        return dpi
    end

    s.data.dpi_cache = get_fallback()
    return s.data.dpi_cache
end

function screen.object.set_dpi(s, dpi)
    s.data.dpi = dpi
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
