---------------------------------------------------------------------------
--- Layout module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module awful.layout
---------------------------------------------------------------------------

-- Grab environment we need
local ipairs = ipairs
local type = type
local capi = {
    screen = screen,
    mouse  = mouse,
    awesome = awesome,
    client = client,
    tag = tag
}
local tag = require("awful.tag")
local client = require("awful.client")
local ascreen = require("awful.screen")
local timer = require("gears.timer")
local gmath = require("gears.math")
local gtable = require("gears.table")
local gdebug = require("gears.debug")
local protected_call = require("gears.protected_call")

local function get_screen(s)
    return s and capi.screen[s]
end

local layout = {}

layout.suit = require("awful.layout.suit")

layout.layouts = {
    layout.suit.floating,
    layout.suit.tile,
    layout.suit.tile.left,
    layout.suit.tile.bottom,
    layout.suit.tile.top,
    layout.suit.fair,
    layout.suit.fair.horizontal,
    layout.suit.spiral,
    layout.suit.spiral.dwindle,
    layout.suit.max,
    layout.suit.max.fullscreen,
    layout.suit.magnifier,
    layout.suit.corner.nw,
    layout.suit.corner.ne,
    layout.suit.corner.sw,
    layout.suit.corner.se,
}

--- The default list of layouts.
--
-- The default value is:
--
--    awful.layout.suit.floating,
--    awful.layout.suit.tile,
--    awful.layout.suit.tile.left,
--    awful.layout.suit.tile.bottom,
--    awful.layout.suit.tile.top,
--    awful.layout.suit.fair,
--    awful.layout.suit.fair.horizontal,
--    awful.layout.suit.spiral,
--    awful.layout.suit.spiral.dwindle,
--    awful.layout.suit.max,
--    awful.layout.suit.max.fullscreen,
--    awful.layout.suit.magnifier,
--    awful.layout.suit.corner.nw,
--    awful.layout.suit.corner.ne,
--    awful.layout.suit.corner.sw,
--    awful.layout.suit.corner.se,
--
-- @field layout.layouts

--- Return the tag layout index (from `awful.layout.layouts`).
--
-- If the layout isn't part of `awful.layout.layouts`, this function returns
-- nil.
--
-- @tparam tag t The tag.
-- @treturn nil|number The layout index.
function layout.get_tag_layout_index(t)
    return gtable.hasitem(layout.layouts, t.layout)
end

-- This is a special lock used by the arrange function.
-- This avoids recurring call by emitted signals.
local arrange_lock = false
-- Delay one arrange call per screen.
local delayed_arrange = {}

--- Get the current layout.
-- @param screen The screen.
-- @return The layout function.
function layout.get(screen)
    screen = screen or capi.mouse.screen
    local t = get_screen(screen).selected_tag
    return tag.getproperty(t, "layout") or layout.suit.floating
end

--- Change the layout of the current tag.
-- @param i Relative index.
-- @param s The screen.
-- @param[opt] layouts A table of layouts.
function layout.inc(i, s, layouts)
    if type(i) == "table" then
        -- Older versions of this function had arguments (layouts, i, s), but
        -- this was changed so that 'layouts' can be an optional parameter
        gdebug.deprecate("Use awful.layout.inc(increment, screen, layouts) instead"..
            " of awful.layout.inc(layouts, increment, screen)", {deprecated_in=5})

        layouts, i, s = i, s, layouts
    end
    s = get_screen(s or ascreen.focused())
    local t = s.selected_tag

    if not t then return end

    layouts = layouts or t.layouts or {}

    if #layouts == 0 then
        layouts = layout.layouts
    end

    local cur_l = layout.get(s)

    -- First try to match the object
    local cur_idx =  gtable.find_first_key(
        layouts, function(_, v) return v == cur_l or cur_l._type == v end, true
    )

    -- Safety net: handle cases where another reference of the layout
    -- might be given (e.g. when (accidentally) cloning it).
    cur_idx = cur_idx or gtable.find_first_key(
        layouts, function(_, v) return v.name == cur_l.name end, true
    )

    -- Trying to come up with some kind of fallback layouts to iterate would
    -- never produce a result the user expect, so if there is nothing to
    -- iterate over, do not iterate.
    if not cur_idx then return end

    local newindex = gmath.cycle(#layouts, cur_idx + i)
    layout.set(layouts[newindex], t)
end

--- Set the layout function of the current tag.
-- @param _layout Layout name.
-- @tparam[opt=mouse.screen.selected_tag] tag t The tag to modify.
function layout.set(_layout, t)
    t = t or capi.mouse.screen.selected_tag
    t.layout = _layout
end

--- Get the layout parameters used for the screen
--
-- This should give the same result as "arrange", but without the "geometries"
-- parameter, as this is computed during arranging.
--
-- If `t` is given, `screen` is ignored, if none are given, the mouse screen is
-- used.
--
-- @tparam[opt] tag t The tag to query
-- @param[opt] screen The screen
-- @treturn table A table with the workarea (x, y, width, height), the screen
--   geometry (x, y, width, height), the clients, the screen and sometime, a
--   "geometries" table with client as keys and geometry as value
function layout.parameters(t, screen)
    screen = get_screen(screen)
    t = t or screen.selected_tag

    screen = get_screen(t and t.screen or 1)

    local p = {}

    local clients           = client.tiled(screen)
    local gap_single_client = true

    if(t and t.gap_single_client ~= nil) then
        gap_single_client = t.gap_single_client
    end

    local useless_gap = 0
    if t then
        local skip_gap = layout.get(screen).skip_gap or function(nclients)
            return nclients < 2
        end
        if gap_single_client or not skip_gap(#clients, t) then
            useless_gap = t.gap
        end
    end

    p.workarea = screen:get_bounding_geometry {
        honor_padding  = true,
        honor_workarea = true,
        margins        = useless_gap,
    }

    p.geometry    = screen.geometry
    p.clients     = clients
    p.screen      = screen.index
    p.padding     = screen.padding
    p.useless_gap = useless_gap

    return p
end

--- Arrange a screen using its current layout.
-- @param screen The screen to arrange.
function layout.arrange(screen)
    screen = get_screen(screen)
    if not screen or delayed_arrange[screen] then return end
    delayed_arrange[screen] = true

    timer.delayed_call(function()
        if not screen.valid then
            -- Screen was removed
            delayed_arrange[screen] = nil
            return
        end
        if arrange_lock then return end
        arrange_lock = true

        -- protected call to ensure that arrange_lock will be reset
        protected_call(function()
            local p = layout.parameters(nil, screen)

            local useless_gap = p.useless_gap

            p.geometries = setmetatable({}, {__mode = "k"})
            layout.get(screen).arrange(p)
            for c, g in pairs(p.geometries) do
                g.width = math.max(1, g.width - c.border_width * 2 - useless_gap * 2)
                g.height = math.max(1, g.height - c.border_width * 2 - useless_gap * 2)
                g.x = g.x + useless_gap
                g.y = g.y + useless_gap
                c:geometry(g)
            end
        end)
        arrange_lock = false
        delayed_arrange[screen] = nil

        screen:emit_signal("arrange")
    end)
end

--- Get the current layout name.
-- @param _layout The layout.
-- @return The layout name.
function layout.getname(_layout)
    _layout = _layout or layout.get()
    return _layout.name
end

local function arrange_prop_nf(obj)
    if not client.object.get_floating(obj) then
        layout.arrange(obj.screen)
    end
end

local function arrange_prop(obj) layout.arrange(obj.screen) end

capi.client.connect_signal("property::size_hints_honor", arrange_prop_nf)
capi.client.connect_signal("property::struts", arrange_prop)
capi.client.connect_signal("property::minimized", arrange_prop_nf)
capi.client.connect_signal("property::sticky", arrange_prop_nf)
capi.client.connect_signal("property::fullscreen", arrange_prop_nf)
capi.client.connect_signal("property::maximized_horizontal", arrange_prop_nf)
capi.client.connect_signal("property::maximized_vertical", arrange_prop_nf)
capi.client.connect_signal("property::border_width", arrange_prop_nf)
capi.client.connect_signal("property::hidden", arrange_prop_nf)
capi.client.connect_signal("property::floating", arrange_prop)
capi.client.connect_signal("property::geometry", arrange_prop_nf)
capi.client.connect_signal("property::screen", function(c, old_screen)
    if old_screen then
        layout.arrange(old_screen)
    end
    layout.arrange(c.screen)
end)

local function arrange_tag(t)
    layout.arrange(t.screen)
end

capi.tag.connect_signal("property::master_width_factor", arrange_tag)
capi.tag.connect_signal("property::master_count", arrange_tag)
capi.tag.connect_signal("property::column_count", arrange_tag)
capi.tag.connect_signal("property::layout", arrange_tag)
capi.tag.connect_signal("property::windowfact", arrange_tag)
capi.tag.connect_signal("property::selected", arrange_tag)
capi.tag.connect_signal("property::activated", arrange_tag)
capi.tag.connect_signal("property::useless_gap", arrange_tag)
capi.tag.connect_signal("property::master_fill_policy", arrange_tag)
capi.tag.connect_signal("tagged", arrange_tag)

capi.screen.connect_signal("property::workarea", layout.arrange)
capi.screen.connect_signal("padding", layout.arrange)

capi.client.connect_signal("focus", function(c)
    local screen = c.screen
    if layout.get(screen).need_focus_update then
        layout.arrange(screen)
    end
end)
capi.client.connect_signal("raised", function(c) layout.arrange(c.screen) end)
capi.client.connect_signal("lowered", function(c) layout.arrange(c.screen) end)
capi.client.connect_signal("list", function()
                                   for screen in capi.screen do
                                       layout.arrange(screen)
                                   end
                               end)

--- Default handler for `request::geometry` signals for tiled clients with
-- the "mouse.move" context.
-- @tparam client c The client
-- @tparam string context The context
-- @tparam table hints Additional hints
function layout.move_handler(c, context, hints) --luacheck: no unused args
    -- Quit if it isn't a mouse.move on a tiled layout, that's handled elsewhere
    if c.floating then return end
    if context ~= "mouse.move" then return end

    if capi.mouse.screen ~= c.screen then
        c.screen = capi.mouse.screen
    end

    local l = c.screen.selected_tag and c.screen.selected_tag.layout or nil
    if l == layout.suit.floating then return end

    local c_u_m = capi.mouse.current_client
    if c_u_m and not c_u_m.floating then
        if c_u_m ~= c then
            c:swap(c_u_m)
        end
    end
end

capi.client.connect_signal("request::geometry", layout.move_handler)

-- When a screen is moved, make (floating) clients follow it
capi.screen.connect_signal("property::geometry", function(s, old_geom)
    local geom = s.geometry
    local xshift = geom.x - old_geom.x
    local yshift = geom.y - old_geom.y
    for _, c in ipairs(capi.client.get(s)) do
        local cgeom = c:geometry()
        c:geometry({
            x = cgeom.x + xshift,
            y = cgeom.y + yshift
        })
    end
end)

return layout

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
