---------------------------------------------------------------------------
--- Implements EWMH requests handling.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.ewmh
---------------------------------------------------------------------------

local setmetatable = setmetatable
local client = client
local screen = screen
local ipairs = ipairs
local math = math
local util = require("awful.util")
local aclient = require("awful.client")
local aplace = require("awful.placement")

local ewmh = {}

local data = setmetatable({}, { __mode = 'k' })

local function screen_change(window)
    if data[window] then
        for _, reqtype in ipairs({ "maximized_vertical", "maximized_horizontal", "fullscreen" }) do
            if data[window][reqtype] then
                if data[window][reqtype].width then
                    data[window][reqtype].width = math.min(data[window][reqtype].width,
                                                           screen[window.screen].workarea.width)
                    if reqtype == "maximized_horizontal" then
                        local bw = window.border_width or 0
                        data[window][reqtype].width = data[window][reqtype].width - 2*bw
                    end
                end
                if data[window][reqtype].height then
                    data[window][reqtype].height = math.min(data[window][reqtype].height,
                                                             screen[window.screen].workarea.height)
                    if reqtype == "maximized_vertical" then
                        local bw = window.border_width or 0
                        data[window][reqtype].height = data[window][reqtype].height - 2*bw
                    end
                end
                if data[window][reqtype].screen then
                    local from = screen[data[window][reqtype].screen].workarea
                    local to = screen[window.screen].workarea
                    local new_x, new_y
                    if data[window][reqtype].x then
                        new_x = to.x + data[window][reqtype].x - from.x
                        if new_x > to.x + to.width then new_x = to.x end
                        -- Move window if it overlaps the new area to the right.
                        if new_x + data[window][reqtype].width > to.x + to.width then
                            new_x = to.x + to.width - data[window][reqtype].width
                        end
                        if new_x < to.x then new_x = to.x end
                        data[window][reqtype].x = new_x
                    end
                    if data[window][reqtype].y then
                        new_y = to.y + data[window][reqtype].y - from.y
                        if new_y > to.y + to.width then new_y = to.y end
                        -- Move window if it overlaps the new area to the bottom.
                        if new_y + data[window][reqtype].height > to.y + to.height then
                            new_y = to.y + to.height - data[window][reqtype].height
                        end
                        if new_y < to.y then new_y = to.y end
                        data[window][reqtype].y = new_y
                    end
                end
            end
        end
    end
end

--- Update a client's settings when its geometry changes, skipping signals
-- resulting from calls within.
local geometry_change_lock = false
local function geometry_change(window)
    if geometry_change_lock then return end
    geometry_change_lock = true

    -- Fix up the geometry in case this window needs to cover the whole screen.
    local bw = window.border_width or 0
    local g = screen[window.screen].workarea
    if window.maximized_vertical then
        window:geometry { height = g.height - 2*bw, y = g.y }
    end
    if window.maximized_horizontal then
        window:geometry { width = g.width - 2*bw, x = g.x }
    end
    if window.fullscreen then
        window.border_width = 0
        window:geometry(screen[window.screen].geometry)
    end

    geometry_change_lock = false
end

--- Activate a window.
--
-- This sets the focus only if the client is visible.
--
-- It is the default signal handler for `request::activate` on a `client`.
--
-- @client c A client to use
-- @tparam string context The context where this signal was used.
-- @tparam[opt] table hints A table with additional hints:
-- @tparam[opt=false] boolean hints.raise should the client be raised?
function ewmh.activate(c, context, hints) -- luacheck: no unused args
    hints = hints or  {}

    if c.focusable == false and not hints.force then return end

    if c:isvisible() then
        client.focus = c
    end
    if hints and hints.raise then
        c:raise()
        if not awesome.startup and not c:isvisible() then
            c.urgent = true
        end
    end
end

--- Tag a window with its requested tag
--
-- @client c A client to tag
-- @tag[opt] t A tag to use. If omitted, then the client is made sticky.
-- @tparam[opt={}] table hints Extra information
function ewmh.tag(c, t, hints) --luacheck: no unused
    -- There is nothing to do
    if not t and #c:tags() > 0 then return end

    if not t then
        c:to_selected_tags()
    elseif type(t) == "boolean" and t then
        c.sticky = true
    else
        c.screen = t.screen
        c:tags({ t })
    end
end

function ewmh.urgent(c, urgent)
    if c ~= client.focus and not aclient.property.get(c,"ignore_urgent") then
        c.urgent = urgent
    end
end

-- Map the state to the action name
local context_mapper = {
    maximized_vertical   = "maximize_vertically",
    maximized_horizontal = "maximize_horizontally",
    fullscreen           = "maximize"
}

--- Move and resize the client.
--
-- This is the default geometry request handler.
--
-- @tparam client c The client
-- @tparam string context The context
-- @tparam[opt={}] table hints The hints to pass to the handler
function ewmh.geometry(c, context, hints)
    context = context or ""

    local original_context = context

    -- Now, map it to something useful
    context = context_mapper[context] or context

    local props = util.table.clone(hints or {}, false)
    props.store_geometry = props.store_geometry==nil and true or props.store_geometry

    -- If it is a known placement function, then apply it, otherwise let
    -- other potential handler resize the client (like in-layout resize or
    -- floating client resize)
    if aplace[context] then

        -- Check if it correspond to a boolean property
        local state = c[original_context]

        -- If the property is boolean and it correspond to the undo operation,
        -- restore the stored geometry.
        if state == false then
            aplace.restore(c,{context=context})
            return
        end

        local honor_default = original_context ~= "fullscreen"

        if props.honor_workarea == nil then
            props.honor_workarea = honor_default
        end

        aplace[context](c, props)
    end
end

client.connect_signal("request::activate", ewmh.activate)
client.connect_signal("request::tag", ewmh.tag)
client.connect_signal("request::urgent", ewmh.urgent)
client.connect_signal("request::geometry", ewmh.geometry)
client.connect_signal("property::screen", screen_change)
client.connect_signal("property::border_width", geometry_change)
client.connect_signal("property::geometry", geometry_change)
screen.connect_signal("property::workarea", function(s)
    for _, c in pairs(client.get(s)) do
        geometry_change(c)
    end
end)

return ewmh

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
