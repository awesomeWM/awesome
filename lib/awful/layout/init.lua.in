---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local ipairs = ipairs
local type = type
local tag = require("awful.tag")
local util = require("awful.util")
local ascreen = require("awful.screen")
local capi = {
    screen = screen,
    awesome = awesome,
    client = client
}
local client = require("awful.client")

--- Layout module for awful
-- awful.layout
local layout = {}
layout.suit = require("awful.layout.suit")

-- This is a special lock used by the arrange function.
-- This avoids recurring call by emitted signals.
local arrange_lock = false

--- Get the current layout.
-- @param screen The screen number.
-- @return The layout function.
function layout.get(screen)
    local t = tag.selected(screen)
    return tag.getproperty(t, "layout") or layout.suit.floating
end

--- Change the layout of the current tag.
-- @param layouts A table of layouts.
-- @param i Relative index.
-- @param s The screen number. 
function layout.inc(layouts, i, s)
    local t = tag.selected(s)
    if t then
        local curlayout = layout.get(s)
        local curindex
        for k, v in ipairs(layouts) do
            if v == curlayout then
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

--- Arrange a screen using its current layout.
-- @param screen The screen to arrange.
function layout.arrange(screen)
    if arrange_lock then return end
    arrange_lock = true
    local p = {}
    p.workarea = capi.screen[screen].workarea
    -- Handle padding
    local padding = ascreen.padding(capi.screen[screen])
    if padding then
        p.workarea.x = p.workarea.x + (padding.left or 0)
        p.workarea.y = p.workarea.y + (padding.top or 0)
        p.workarea.width = p.workarea.width - ((padding.left or 0 ) + (padding.right or 0))
        p.workarea.height = p.workarea.height - ((padding.top or 0) + (padding.bottom or 0))
    end
    p.geometry = capi.screen[screen].geometry
    p.clients = client.tiled(screen)
    p.screen = screen
    layout.get(screen).arrange(p)
    capi.screen[screen]:emit_signal("arrange")
    arrange_lock = false
end

--- Get the current layout name.
-- @param layout The layout.
-- @return The layout name.
function layout.getname(_layout)
    local _layout = _layout or layout.get()
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
-- If prop is screen, we do not know what was the previous screen, so
-- let's arrange all screens :-(
capi.client.connect_signal("property::screen", function(c)
    for screen = 1, capi.screen.count() do layout.arrange(screen) end end)

local function arrange_on_tagged(c, tag)
    if not tag.screen then return end
    layout.arrange(tag.screen)
    if not capi.client.focus or not capi.client.focus:isvisible() then
        local c = client.focus.history.get(tag.screen, 0)
        if c then capi.client.focus = c end
    end
end
local function arrange_tag(t)
    layout.arrange(tag.getscreen(t))
end

for s = 1, capi.screen.count() do
    capi.screen[s]:add_signal("arrange")

    tag.attached_connect_signal(s, "property::mwfact", arrange_tag)
    tag.attached_connect_signal(s, "property::nmaster", arrange_tag)
    tag.attached_connect_signal(s, "property::ncol", arrange_tag)
    tag.attached_connect_signal(s, "property::layout", arrange_tag)
    tag.attached_connect_signal(s, "property::windowfact", arrange_tag)
    tag.attached_connect_signal(s, "property::selected", arrange_tag)
    tag.attached_connect_signal(s, "property::activated", arrange_tag)
    tag.attached_connect_signal(s, "tagged", arrange_tag)
    capi.screen[s]:connect_signal("property::workarea", function(screen)
        layout.arrange(screen.index)
    end)
    capi.screen[s]:connect_signal("padding", function (screen)
        layout.arrange(screen.index)
    end)
end

capi.client.connect_signal("focus", function(c) layout.arrange(c.screen) end)
capi.client.connect_signal("list", function()
                                   for screen = 1, capi.screen.count() do
                                       layout.arrange(screen)
                                   end
                               end)

return layout

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
