---------------------------------------------------------------------------
--- Layout module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.layout
---------------------------------------------------------------------------

-- Grab environment we need
local ipairs = ipairs
local type = type
local util = require("awful.util")
local ascreen = require("awful.screen")
local capi = {
    screen = screen,
    mouse  = mouse,
    awesome = awesome,
    client = client,
    tag = tag
}
local tag = require("awful.tag")
local client = require("awful.client")
local timer = require("gears.timer")

local function get_screen(s)
    return s and capi.screen[s]
end

local layout = {}

--- Default predefined layouts
--
-- @fixme Add documentation on available layouts as all of them are hidden
layout.suit = require("awful.layout.suit")

-- The default list of layouts
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

-- This is a special lock used by the arrange function.
-- This avoids recurring call by emitted signals.
local arrange_lock = false
-- Delay one arrange call per screen.
local delayed_arrange = {}

--- Get the current layout.
-- @param screen The screen.
-- @return The layout function.
function layout.get(screen)
    screen = get_screen(screen)
    local t = tag.selected(screen and screen.index)
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
        layouts, i, s = i, s, layouts
    end
    s = get_screen(s)
    local t = tag.selected(s and s.index)
    layouts = layouts or layout.layouts
    if t then
        local curlayout = layout.get(s)
        local curindex
        for k, v in ipairs(layouts) do
            if v == curlayout or curlayout._type == v then
                curindex = k
                break
            end
        end
        if not curindex then
            -- Safety net: handle cases where another reference of the layout
            -- might be given (e.g. when (accidentally) cloning it).
            for k, v in ipairs(layouts) do
                if v.name == curlayout.name then
                    curindex = k
                    break
                end
            end
        end
        if curindex then
            local newindex = util.cycle(#layouts, curindex + i)
            layout.set(layouts[newindex], t)
        end
    end
end

--- Set the layout function of the current tag.
-- @param _layout Layout name.
-- @param t The tag to modify, if null tag.selected() is used.
function layout.set(_layout, t)
    t = t or tag.selected()
    tag.setproperty(t, "layout", _layout)
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
    t = t or tag.selected(screen and screen.index)

    if not t then return end

    screen = get_screen(tag.getscreen(t) or 1)

    local p = {}

    p.workarea = screen.workarea

    local useless_gap = tag.getgap(t, #client.tiled(screen.index))

    -- Handle padding
    local padding = ascreen.padding(screen) or {}

    p.workarea.x = p.workarea.x + (padding.left or 0) + useless_gap

    p.workarea.y = p.workarea.y + (padding.top or 0) + useless_gap

    p.workarea.width = p.workarea.width - ((padding.left or 0 ) +
        (padding.right or 0) + useless_gap * 2)

    p.workarea.height = p.workarea.height - ((padding.top or 0) +
        (padding.bottom or 0) + useless_gap * 2)

    p.geometry    = screen.geometry
    p.clients     = client.tiled(screen)
    p.screen      = screen.index
    p.padding     = padding
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
        if arrange_lock then return end
        arrange_lock = true

        local p = layout.parameters(nil, screen)

        local useless_gap = p.useless_gap

        p.geometries = setmetatable({}, {__mode = "k"})
        layout.get(screen).arrange(p)
        for c, g in pairs(p.geometries) do
            g.width = g.width - c.border_width * 2 - useless_gap * 2
            g.height = g.height - c.border_width * 2 - useless_gap * 2
            g.x = g.x + useless_gap
            g.y = g.y + useless_gap
            c:geometry(g)
        end
        screen:emit_signal("arrange")

        arrange_lock = false
        delayed_arrange[screen] = nil
    end)
end

--- Get the current layout name.
-- @param _layout The layout.
-- @return The layout name.
function layout.getname(_layout)
    _layout = _layout or layout.get()
    return _layout.name
end

local function arrange_prop(obj) layout.arrange(obj.screen) end

capi.client.connect_signal("property::size_hints_honor", arrange_prop)
capi.client.connect_signal("property::struts", arrange_prop)
capi.client.connect_signal("property::minimized", arrange_prop)
capi.client.connect_signal("property::sticky", arrange_prop)
capi.client.connect_signal("property::fullscreen", arrange_prop)
capi.client.connect_signal("property::maximized_horizontal", arrange_prop)
capi.client.connect_signal("property::maximized_vertical", arrange_prop)
capi.client.connect_signal("property::border_width", arrange_prop)
capi.client.connect_signal("property::hidden", arrange_prop)
capi.client.connect_signal("property::floating", arrange_prop)
capi.client.connect_signal("property::geometry", arrange_prop)
capi.client.connect_signal("property::screen", function(c, old_screen)
    if old_screen then
        layout.arrange(old_screen)
    end
    layout.arrange(c.screen)
end)

local function arrange_tag(t)
    layout.arrange(tag.getscreen(t))
end

capi.screen.add_signal("arrange")

capi.tag.connect_signal("property::mwfact", arrange_tag)
capi.tag.connect_signal("property::nmaster", arrange_tag)
capi.tag.connect_signal("property::ncol", arrange_tag)
capi.tag.connect_signal("property::layout", arrange_tag)
capi.tag.connect_signal("property::windowfact", arrange_tag)
capi.tag.connect_signal("property::selected", arrange_tag)
capi.tag.connect_signal("property::activated", arrange_tag)
capi.tag.connect_signal("property::useless_gap", arrange_tag)
capi.tag.connect_signal("property::master_fill_policy", arrange_tag)
capi.tag.connect_signal("tagged", arrange_tag)

for s = 1, capi.screen.count() do
    capi.screen[s]:connect_signal("property::workarea", function(screen)
        layout.arrange(screen.index)
    end)
    capi.screen[s]:connect_signal("padding", function (screen)
        layout.arrange(screen.index)
    end)
end

capi.client.connect_signal("raised", function(c) layout.arrange(c.screen) end)
capi.client.connect_signal("lowered", function(c) layout.arrange(c.screen) end)
capi.client.connect_signal("list", function()
                                   for screen = 1, capi.screen.count() do
                                       layout.arrange(screen)
                                   end
                               end)

return layout

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
