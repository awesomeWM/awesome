---------------------------------------------------------------------------
--- Screen module for awful.
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
    root = root,
}
local gdebug = require("gears.debug")
local gmath = require("gears.math")
local object = require("gears.object")
local grect =  require("gears.geometry").rectangle
local gsurf = require("gears.surface")
local cairo = require("lgi").cairo

local function get_screen(s)
    return s and capi.screen[s]
end

-- we use require("awful.client") inside functions to prevent circular dependencies.
local client

local screen = {object={}}

local data = {}
data.padding = {}

--- Take an input geometry and subtract/add a delta.
-- @tparam table geo A geometry (width, height, x, y) table.
-- @tparam table delta A delta table (top, bottom, x, y).
-- @treturn table A geometry (width, height, x, y) table.
local function apply_geometry_adjustments(geo, delta)
    return {
        x      = geo.x      + (delta.left or 0),
        y      = geo.y      + (delta.top  or 0),
        width  = geo.width  - (delta.left or 0) - (delta.right  or 0),
        height = geo.height - (delta.top  or 0) - (delta.bottom or 0),
    }
end

--- Get the square distance between a `screen` and a point.
-- @deprecated awful.screen.getdistance_sq
-- @tparam screen s Screen
-- @tparam integer x X coordinate of point
-- @tparam integer y Y coordinate of point
-- @treturn number The squared distance of the screen to the provided point.
-- @see screen.get_square_distance
function screen.getdistance_sq(s, x, y)
    gdebug.deprecate("Use s:get_square_distance(x, y) instead of awful.screen.getdistance_sq", {deprecated_in=4})
    return screen.object.get_square_distance(s, x, y)
end

--- Get the square distance between a `screen` and a point.
-- @method get_square_distance
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
-- @staticfct awful.screen.getbycoord
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
-- @staticfct awful.screen.focus
-- @tparam screen screen Screen number (defaults / falls back to mouse.screen).
-- @treturn screen The newly focused screen.
-- @request client activate screen.focus granted The most recent focused client
--  for this screen should be re-activated.
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

    return _screen
end

--- Get the next screen in a specific direction.
--
-- This gets the next screen relative to this one in
-- the specified direction.
--
-- @method get_next_in_direction
-- @tparam string dir The direction, can be either "up", "down", "left" or "right".
-- @treturn screen The next screen.
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
-- @staticfct awful.screen.focus_bydirection
-- @tparam string dir The direction, can be either "up", "down", "left" or "right".
-- @tparam[opt=awful.screen.focused()] screen s Screen.
-- @treturn screen The next screen.
function screen.focus_bydirection(dir, s)
    local sel = get_screen(s or screen.focused())
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
-- @staticfct awful.screen.focus_relative
-- @tparam int offset Value to add to the current focused screen index. 1 to
--   focus the next one, -1 to focus the previous one.
-- @treturn screen The newly focusd screen.
function screen.focus_relative(offset)
    return screen.focus(gmath.cycle(capi.screen.count(),
                                   screen.focused().index + offset))
end

--- The area where clients can be tiled.
--
-- This property holds the area where clients can be tiled. Use
-- the `padding` property, `wibox.struts` and `client.struts` to modify this
-- area.
--
-- @DOC_screen_tiling_area_EXAMPLE@
--
-- @property tiling_area
-- @tparam table tiling_area
-- @tparam number tiling_area.x
-- @tparam number tiling_area.y
-- @tparam number tiling_area.width
-- @tparam number tiling_area.height
-- @propertydefault This is the `workarea` substracted with the `padding` area.
-- @propertyunit pixel
-- @readonly
-- @see padding
-- @see get_bounding_geometry

function screen.object.get_tiling_area(s)
    return s:get_bounding_geometry {
        honor_padding  = true,
        honor_workarea = true,
    }
end

--- Take a screenshot of the physical screen.
--
-- Reading this property returns a screenshot of the physical
-- (Xinerama) screen as a cairo surface.
--
-- Use `gears.surface(c.content)` to convert the raw content into a static image.
--
-- @property content
-- @tparam raw_surface content
-- @propertydefault The client raw pixels at the time the property is called.
--  If there is no compositing manager running, it might be black.
-- @see gears.surface
-- @readonly

function screen.object.get_content(s)
    local geo = s.geometry
    local source = gsurf(capi.root.content())
    local target = source:create_similar(cairo.Content.COLOR, geo.width,
                                         geo.height)
    local cr = cairo.Context(target)
    cr:set_source_surface(source, -geo.x, -geo.y)
    cr:rectangle(0, 0, geo.width, geo.height)
    cr:fill()
    return target
end

--- Get or set the screen padding.
--
-- @deprecated awful.screen.padding
-- @tparam screen s The screen object to change the padding on.
-- @tparam[opt=nil] table|integer|nil padding The padding, a table with 'top', 'left', 'right' and/or
-- 'bottom' or a number value to apply set the same padding on all sides. Can be
--  nil if you only want to retrieve padding
-- @treturn table A table with left, right, top and bottom number values.
-- @see padding
function screen.padding(s, padding)
    gdebug.deprecate("Use _screen.padding = value instead of awful.screen.padding", {deprecated_in=4})
    if padding then
        screen.object.set_padding(s, padding)
    end
    return screen.object.get_padding(s)
end

--- The screen padding.
--
-- This adds a "buffer" section on each side of the screen.
--
-- @DOC_screen_padding_EXAMPLE@
--
-- @property padding
-- @tparam[opt=0] table|number padding
-- @tparam[opt=0] integer padding.left The padding on the left.
-- @tparam[opt=0] integer padding.right The padding on the right.
-- @tparam[opt=0] integer padding.top The padding on the top.
-- @tparam[opt=0] integer padding.bottom The padding on the bottom.
-- @negativeallowed false
-- @propertyunit pixel
-- @propertytype number A single value for each sides.
-- @propertytype table A different value for each sides.
-- @propemits true false
-- @usebeautiful beautiful.maximized_honor_padding Honor the screen padding when maximizing.

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

--- A list of outputs for this screen with their size in mm.
--
-- Please note that the table content may vary. In some case, it might also be
-- empty.
--
-- An easy way to check if a screen is the laptop screen is usually:
--
--    if s.outputs["LVDS-1"] then
--        -- do something
--    end
--
-- @property outputs
-- @tparam table outputs
-- @tablerowtype A key-value table with the output name as key and a table of
--  metadata as value.
-- @tablerowkey integer mm_width The screen physical width.
-- @tablerowkey integer mm_height The screen physical height.
-- @tablerowkey string name The output name.
-- @tablerowkey string viewport_id The identifier of the viewport this output
--  corresponds to.
-- @propertydefault This may or may not be populated if the screen are based on
--  an actual physical screen. For fake screen, this property content is undefined.
-- @propemits true false
-- @readonly

function screen.object.get_outputs(s)
    local ret = {}

    local outputs = s._custom_outputs
        or (s._private.viewport and s._private.viewport.outputs or s._outputs)

    -- The reason this exists is because output with name as keys is very
    -- convenient for quick name lookup by the users, but inconvenient in
    -- the lower layers since knowing the output count (using #) is better.
    for k, v in ipairs(outputs) do
        ret[v.name or k] = v
    end

    return ret
end

function screen.object.set_outputs(self, outputs)
    self._custom_outputs = outputs
    self:emit_signal("property::outputs", screen.object.get_outputs(self))
end

capi.screen.connect_signal("property::_outputs", function(s)
    if not s._custom_outputs then
        s:emit_signal("property::outputs", screen.object.get_outputs(s))
    end
end)

--- Get the preferred screen in the context of a client.
--
-- This is exactly the same as `awful.screen.focused` except that it avoids
-- clients being moved when Awesome is restarted.
-- This is used in the default `rc.lua` to ensure clients get assigned to the
-- focused screen by default.
-- @tparam client c A client.
-- @treturn screen The preferred screen.
-- @staticfct awful.screen.preferred
function screen.preferred(c)
    return capi.awesome.startup and c.screen or screen.focused()
end

--- The defaults arguments for `awful.screen.focused`.
-- @tfield[opt={}] table awful.screen.default_focused_args

--- Get the focused screen.
--
-- It is possible to set `awful.screen.default_focused_args` to override the
-- default settings.
--
-- @staticfct awful.screen.focused
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
-- @method screen.get_bounding_geometry
-- @tparam[opt={}] table args The arguments
-- @tparam[opt=false] boolean args.honor_padding Whether to honor the screen's padding.
-- @tparam[opt=false] boolean args.honor_workarea Whether to honor the screen's workarea.
-- @tparam[opt] int|table args.margins Apply some margins on the output.
--   This can either be a number or a table with *left*, *right*, *top*
--   and *bottom* keys.
-- @tparam[opt] tag args.tag Use this tag's screen.
-- @tparam[opt] drawable args.parent A parent drawable to use as base geometry.
-- @tparam[opt] table args.bounding_rect A bounding rectangle. This parameter is
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
        geo = apply_geometry_adjustments(geo, padding)
    end

    if args.margins then
        geo = apply_geometry_adjustments(geo,
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
-- @tparam[opt={}] table clients The clients list, ordered from top to bottom.
-- @tablerowtype A list of `client` objects.
-- @see all_clients
-- @see hidden_clients
-- @see client.get

--- Get the list of visible clients for the screen.
--
-- This is used by `screen.clients` internally (with `stacked=true`).
--
-- @method get_clients
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
-- @tparam[opt={}] table hidden_clients The clients list, ordered from top to bottom.
-- @tablerowtype A list of `client` objects.
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
-- @tparam[opt={}] table all_clients The clients list, ordered from top to bottom.
-- @tablerowtype A list of `client` objects.
-- @see clients
-- @see hidden_clients
-- @see client.get

--- Get all clients assigned to the screen.
--
-- This is used by `all_clients` internally (with `stacked=true`).
--
-- @method get_all_clients
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
-- @DOC_screen_tiled_clients_EXAMPLE@
--
-- @property tiled_clients
-- @tparam[opt={}] table tiled_clients The clients list, ordered from top to bottom.
-- @tablerowtype A list of `client` objects.

--- Get tiled clients for the screen.
--
-- This is used by `tiles_clients` internally (with `stacked=true`).
--
-- @method get_tiled_clients
-- @tparam[opt=true] boolean stacked Use stacking order? (top to bottom)
-- @treturn table The clients list.
function screen.object.get_tiled_clients(s, stacked)
    local clients = s:get_clients(stacked)
    local tclients = {}
    -- Remove floating clients
    for _, c in pairs(clients) do
        if not c.floating
            and not c.immobilized_horizontal
            and not c.immobilized_vertical then
            table.insert(tclients, c)
        end
    end
    return tclients
end

--- Call a function for each existing and created-in-the-future screen.
--
-- @staticfct awful.screen.connect_for_each_screen
-- @tparam function func The function to call.
-- @tparam screen func.screen The screen.
-- @noreturn
function screen.connect_for_each_screen(func)
    for s in capi.screen do
        func(s)
    end
    capi.screen.connect_signal("added", func)
end

--- Undo the effect of `awful.screen.connect_for_each_screen`.
-- @staticfct awful.screen.disconnect_for_each_screen
-- @tparam function func The function that should no longer be called.
-- @noreturn
function screen.disconnect_for_each_screen(func)
    capi.screen.disconnect_signal("added", func)
end

--- A list of all tags on the screen.
--
-- Use `tag.screen`, `awful.tag.add`,
-- `awful.tag.new` or `t:delete()` to alter this list.
--
-- @property tags
-- @tparam[opt={}] table tags
-- @tablerowtype A table with all available tags.
-- @readonly

function screen.object.get_tags(s, unordered)
    local tags = {}

    for _, t in ipairs(capi.root.tags()) do
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
-- @tparam[opt={}] table selected_tags
-- @tablerowtype A table with all selected tags.
-- @readonly
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
-- @tparam[opt=nil] tag|nil selected_tag
-- @readonly
-- @see tag.selected
-- @see selected_tags

function screen.object.get_selected_tag(s)
    return screen.object.get_selected_tags(s)[1]
end

local function normalize(ratios, size)
    local sum = 0

    for _, r in ipairs(ratios) do
        sum = sum + r
    end

    -- Avoid to mutate the input.
    local ret = {}
    local sum2 = 0

    for k, r in ipairs(ratios) do
        ret[k] = (r*100)/sum
        ret[k] = math.floor(size*ret[k]*0.01)
        sum2 = sum2 + ret[k]
    end

    -- Ratios are random float number. Pixels cannot be divided. This adds the
    -- remaining pixels to the end. A better approach would be to redistribute
    -- them based on the ratios. However, nobody will notice.
    ret[#ret] = ret[#ret] + (size - sum2)

    return ret
end

--- Split the screen into multiple screens.
--
-- This is useful to turn ultrawide monitors into something more useful without
-- fancy client layouts:
--
-- @DOC_awful_screen_split1_EXAMPLE@
--
-- It can also be used to turn a vertical "side" screen into 2 smaller screens:
--
-- @DOC_awful_screen_split2_EXAMPLE@
--
-- @tparam[opt={50﹐50}] table ratios The different ratios to split into. If none
--  is provided, it is split in half.
-- @tparam[opt] string mode Either "vertical" or "horizontal". If none is
--  specified, it will split along the longest axis.
-- @treturn table A table with the screen objects. The first value is the
--  original screen object (`s`) and the following one(s) are the new screen
--  objects. The values are ordered from left to right or top to bottom depending
--  on the value of `mode`.
-- @method split
function screen.object.split(s, ratios, mode, _geo)
    s = get_screen(s)

    _geo = _geo or s.geometry
    ratios = ratios or {50,50}

    -- In practice, this is almost always what the user wants.
    mode = mode or (
        _geo.height > _geo.width and "vertical" or "horizontal"
    )

    assert(mode == "horizontal" or mode == "vertical")

    assert((not s) or s.valid)
    assert(#ratios >= 2)

    local sizes, ret = normalize(
        ratios, mode == "horizontal" and _geo.width or _geo.height
    ), {}

    assert(#sizes >=2)

    if s then
        if mode == "horizontal" then
            s:fake_resize(_geo.x, _geo.y, sizes[1], _geo.height)
        else
            s:fake_resize(_geo.x, _geo.y, _geo.width, sizes[1])
        end
        table.insert(ret, s)
    end

    local pos = _geo[mode == "horizontal" and "x" or "y"]
        + (s and sizes[1] or 0)

    for k=2, #sizes do
        local ns

        if mode == "horizontal" then
            ns = capi.screen.fake_add(pos, _geo.y, sizes[k], _geo.height)
        else
            ns = capi.screen.fake_add(_geo.x, pos, _geo.width, sizes[k])
        end

        table.insert(ret, ns)

        if s then
            ns._private.viewport = s._private.viewport

            if not ns._private.viewport then
                ns.outputs = s.outputs
            end
        end

        pos = pos + sizes[k]
    end

    return ret
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
-- @noreturn
-- @staticfct awful.screen.set_auto_dpi_enabled
function screen.set_auto_dpi_enabled(enabled)
    for s in capi.screen do
        s._private.dpi_cache = nil
    end
    data.autodpi = enabled
end

--- The number of pixels per inch of the screen.
--
-- The default DPI comes from the X11 server. In most case, it will be 96. If
-- `autodpi` is set to `true` on the screen, it will use the least dense dpi
-- from the screen outputs. Most of the time, screens only have a single output,
-- however it will have two (or more) when "clone mode" is used (eg, when a
-- screen is duplicated on a projector).
--
-- @property dpi
-- @tparam[opt=96] number dpi
-- @propertyunit pixel\_per\_inch
-- @negativeallowed false

--- The lowest density DPI from all of the (physical) outputs.
-- @property minimum_dpi
-- @tparam number minimum_dpi
-- @propertyunit pixel\_per\_inch
-- @propertydefault This is extracted from `outputs`.
-- @negativeallowed false
-- @readonly

--- The highest density DPI from all of the (physical) outputs.
-- @property maximum_dpi
-- @tparam number maximum_dpi
-- @propertydefault This is extracted from `outputs`.
-- @propertyunit pixel\_per\_inch
-- @negativeallowed false
-- @readonly

--- The preferred DPI from all of the (physical) outputs.
--
-- This is computed by normalizing all output to fill the area, then picking
-- the lowest of the resulting virtual DPIs.
--
-- @property preferred_dpi
-- @tparam number preferred_dpi
-- @propertydefault This is extracted from `outputs`.
-- @negativeallowed false
-- @readonly

--- The maximum diagonal size in millimeters.
--
-- @property mm_maximum_size
-- @tparam number mm_maximum_size
-- @propertydefault This is extracted from `outputs`.
-- @propertyunit millimeter
-- @negativeallowed false

--- The minimum diagonal size in millimeters.
--
-- @property mm_minimum_size
-- @tparam number mm_minimum_size
-- @propertydefault This is extracted from `outputs`.
-- @propertyunit millimeter
-- @negativeallowed false

--- The maximum diagonal size in inches.
--
-- @property inch_maximum_size
-- @tparam number inch_maximum_size
-- @propertydefault This is extracted from `outputs`.
-- @propertyunit inch
-- @negativeallowed false

--- The minimum diagonal size in inches.
--
-- @property inch_minimum_size
-- @tparam number inch_minimum_size
-- @propertydefault This is extracted from `outputs`.
-- @propertyunit inch
-- @negativeallowed false

--- Emitted when a new screen is added.
--
-- The handler(s) of this signal are responsible of adding elements such as
-- bars, docks or other elements to a screen. The signal is emitted when a
-- screen is added, including during startup.
--
-- The only default implementation is the one provided by `rc.lua`.
--
-- @signal request::desktop_decoration
-- @tparam string context The context.
-- @request screen wallpaper added granted When the decorations needs to be
--  added to a new screen.

--- Emitted when a new screen needs a wallpaper.
--
-- The handler(s) of this signal are responsible to set the wallpaper. The
-- signal is emitted when a screen is added (including at startup), when its
-- DPI changes or when its geometry changes.
--
-- The only default implementation is the one provided by `rc.lua`.
--
-- @signal request::wallpaper
-- @tparam string context The context.
-- @request screen wallpaper added granted When the wallpaper needs to be
--  added to a new screen.
-- @request screen wallpaper geometry granted When the wallpaper needs to be
--  updated because the resolution changed.
-- @request screen wallpaper dpi granted When the wallpaper needs to be
--  updated because the DPI changed.

--- When a new (physical) screen area has been added.
--
-- Important: This only exists when Awesome is started with `--screen off`.
-- Please also note that this doesn't mean it will appear when a screen is
-- physically plugged. Depending on the configuration a tool like `arandr` or
-- the `xrandr` command is needed.
--
-- The default handler will create a screen that fills the area.
--
-- To disconnect the default handler, use:
--
--    screen.disconnect_signal(
--        "request::create", awful.screen.create_screen_handler
--    )
--
-- @signal request::create
-- @tparam table viewport
-- @tparam table viewport.geometry A table with `x`, `y`, `width` and `height`
--  keys.
-- @tparam table viewport.outputs A table with the monitor name and possibly the
--  `mm_width` and `mm_height` values if they are available.
-- @tparam number viewport.id An identifier for this viewport (by pixel
--  resolution). It
--  will not change when outputs are modified, but will change when the
--  resolution changes. Note that if it fully disappear, the next time an
--  viewport with the same resolution appears, it will have a different `id`.
-- @tparam number viewport.minimum_dpi The least dense DPI.
-- @tparam number viewport.maximum_dpi The most dense DPI.
-- @tparam number viewport.preferred_dpi The relative least dense DPI.
-- @tparam table args
-- @tparam string args.context Why was this signal sent.
-- @see outputs
-- @see awful.screen.create_screen_handler

--- When a physical monitor viewport has been removed.
--
-- Important: This only exists when Awesome is started with `--screen off`.
--
-- If you replace the default handler, it is up to you to find the screen(s)
-- associated with this viewport.
--
-- To disconnect the default handler, use:
--
--    screen.disconnect_signal(
--        "request::remove", awful.screen.remove_screen_handler
--    )
--
-- @signal request::remove
-- @tparam table viewport
-- @tparam table viewport.geometry A table with `x`, `y`, `width` and `height`
--  keys.
-- @tparam table viewport.outputs A table with the monitor name and possibly the
--  `mm_width` and `mm_height` values if they are available.
-- @tparam number viewport.id An identifier for this viewport (by pixel
--  resolution). It will not change when outputs are modified, but will change
--  when the resolution changes. Note that if it fully disappear, the next time
--  an viewport with the same resolution appears, it will have a different `id`.
-- @tparam number viewport.minimum_dpi The least dense DPI.
-- @tparam number viewport.maximum_dpi The most dense DPI.
-- @tparam number viewport.preferred_dpi The relative least dense DPI.
-- @tparam table args
-- @tparam string args.context Why was this signal sent.
-- @see awful.screen.remove_screen_handler

--- When a physical viewport resolution has changed or it has been replaced.
--
-- Important: This only exists when Awesome is started with `--screen off`.
--
-- Note that given the viewports are not the same, the `id` won't be the same.
-- Also note that if multiple new viewports fit within a single "old" viewport,
-- the resized screen will be the one with the largest total overlapping
-- viewport (`intersection.width*intersection.height`), regardless of the
-- outputs names.
--
-- To disconnect the default handler, use:
--
--    screen.disconnect_signal(
--        "request::resize", awful.screen.resize_screen_handler
--    )
--
-- @signal request::resize
-- @tparam table old_viewport
-- @tparam table old_viewport.geometry A table with `x`, `y`, `width` and
--  `height` keys.
-- @tparam table old_viewport.outputs A table with the monitor name and
--  possibly the `mm_width` and `mm_height` values if they are available.
-- @tparam number old_viewport.id An identifier for this viewport (by pixel
--  resolution). It will not change when outputs are modified, but will change
--  when the resolution changes. Note that if it fully disappear, the next
--  time an viewport with the same resolution appears, it will have a different
--  `id`.
-- @tparam number old_viewport.minimum_dpi The least dense DPI.
-- @tparam number old_viewport.maximum_dpi The most dense DPI.
-- @tparam number old_viewport.preferred_dpi The relative least dense DPI.
-- @tparam table new_viewport
-- @tparam table new_viewport.geometry A table with `x`, `y`, `width` and
--  `height` keys.
-- @tparam table new_viewport.outputs A table with the monitor name and
--  possibly the
--  `mm_width` and `mm_height` values if they are available.
-- @tparam number new_viewport.id An identifier for this viewport (by pixel
--  resolution). It will not change when outputs are modified, but will change
--  when the  resolution changes. Note that if it fully disappear, the next time
--  an viewport with the same resolution appears, it will have a different `id`.
-- @tparam number new_viewport.minimum_dpi The least dense DPI.
-- @tparam number new_viewport.maximum_dpi The most dense DPI.
-- @tparam number new_viewport.preferred_dpi The relative least dense DPI.
-- @tparam table args
-- @tparam string args.context Why was this signal sent.
-- @see awful.screen.resize_screen_handler

--- Default handler for `request::create`.
--
-- Important: This only exists when Awesome is started with `--screen off`.
--
-- A simplified implementation looks like:
--
--    function(viewport --[[, args]])
--        local geo = viewport.geometry
--        local s = screen.fake_add(geo.x, geo.y, geo.width, geo.height)
--        s:emit_signal("request::desktop_decoration")
--        s:emit_signal("request::wallpaper")
--    end
--
-- If you implement this by hand, you must also implement handler for the
-- `request::remove` and `request::resize`.
--
-- @signalhandler awful.screen.create_screen_handler
-- @tparam table viewport
-- @sourcesignal screen request::create
-- @see request::create

--- Default handler for `request::remove`.
--
-- Important: This only exists when Awesome is started with `--screen off`.
--
-- A simplified version of the logic is:
--
--    function (viewport --[[, args]])
--        local geo = viewport.geometry
--        for s in screen do
--            if gears.geometry.rectangle.are_equal(geo, s.geometry) then
--                s:fake_remove()
--                return
--            end
--        end
--    end
--
-- @signalhandler awful.screen.remove_screen_handler
-- @tparam table viewport
-- @sourcesignal screen request::remove
-- @see request::remove

--- Default handler for `request::resize`.
--
-- Important: This only exists when Awesome is started with `--screen off`.
--
-- A simplified version of the logic is:
--
--    function (old_viewport, new_viewport --[[, args]])
--        local old_geo, new_geo = old_viewport.geometry, new_viewport.geometry
--        for s in screen do
--            local sgeo = new_viewport.geometry
--            if gears.geometry.rectangle.are_equal(old_geo, s.geometry) then
--                s:fake_resize(
--                    sgeo.x, sgeo.y, sgeo.width, sgeo.height
--                )
--            end
--        end
--    end
--
-- @signalhandler awful.screen.resize_screen_handler
-- @tparam table viewport
-- @sourcesignal screen request::resize
-- @see request::resize

-- Add the DPI properties.
require("awful.screen.dpi")(screen, data)

-- Set the wallpaper(s) and create the bar(s) for new screens

capi.screen.connect_signal("_added", function(s)
    -- If it was emitted from here when screens are created with fake_add,
    -- the Lua code would not have an opportunity to polutate the screen
    -- metadata. Thus, the DPI may be wrong when setting the wallpaper.
    if s._managed ~= "Lua" then
        s:emit_signal("added")
        s:emit_signal("request::desktop_decoration", "added")
        s:emit_signal("request::wallpaper", "added")
    end
end)

-- Resize the wallpaper(s)
for _, prop in ipairs {"geometry", "dpi" } do
    capi.screen.connect_signal("property::"..prop, function(s)
        s:emit_signal("request::wallpaper", prop)
    end)
end

-- Create the bar for existing screens when an handler is added
capi.screen.connect_signal("request::desktop_decoration::connected", function(new_handler)
    if capi.screen.automatic_factory then
        for s in capi.screen do
            new_handler(s)
        end
    end
end)

-- Set the wallpaper when an handler is added.
capi.screen.connect_signal("request::wallpaper::connected", function(new_handler)
    if capi.screen.automatic_factory then
        for s in capi.screen do
            new_handler(s)
        end
    end
end)

--- When the tag history changed.
-- @signal tag::history::update

-- Extend the luaobject
object.properties(capi.screen, {
    getter_class = screen.object,
    setter_class = screen.object,
    auto_emit    = true,
})

--@DOC_object_COMMON@

return screen

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
