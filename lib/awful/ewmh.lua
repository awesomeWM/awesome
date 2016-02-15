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
local atag = require("awful.tag")
local aclient = require("awful.client")

local ewmh = {}

local data = setmetatable({}, { __mode = 'k' })

local function store_geometry(window, reqtype)
    if not data[window] then data[window] = {} end
    if not data[window][reqtype] then data[window][reqtype] = {} end
    data[window][reqtype] = window:geometry()
    data[window][reqtype].screen = window.screen
end

--- Maximize a window horizontally.
--
-- @param window The window.
-- @param set Set or unset the maximized values.
local function maximized_horizontal(window, set)
    if set then
        store_geometry(window, "maximized_horizontal")
        local g = screen[window.screen].workarea
        local bw = window.border_width or 0
        window:geometry { width = g.width - 2*bw, x = g.x }
    elseif data[window] and data[window].maximized_horizontal
        and data[window].maximized_horizontal.x
        and data[window].maximized_horizontal.width then
        local g = data[window].maximized_horizontal
        window:geometry { width = g.width, x = g.x }
    end
end

--- Maximize a window vertically.
--
-- @param window The window.
-- @param set Set or unset the maximized values.
local function maximized_vertical(window, set)
    if set then
        store_geometry(window, "maximized_vertical")
        local g = screen[window.screen].workarea
        local bw = window.border_width or 0
        window:geometry { height = g.height - 2*bw, y = g.y }
    elseif data[window] and data[window].maximized_vertical
        and data[window].maximized_vertical.y
        and data[window].maximized_vertical.height then
        local g = data[window].maximized_vertical
        window:geometry { height = g.height, y = g.y }
    end
end

--- Fullscreen a window.
--
-- @param window The window.
-- @param set Set or unset the fullscreen values.
local function fullscreen(window, set)
    if set then
        store_geometry(window, "fullscreen")
        data[window].fullscreen.border_width = window.border_width
        window.border_width = 0
        window:geometry(screen[window.screen].geometry)
    elseif data[window] and data[window].fullscreen then
        window.border_width = data[window].fullscreen.border_width
        window:geometry(data[window].fullscreen)
    end
end

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
function ewmh.tag(c, t)
    if not t then
        c.sticky = true
    else
        c.screen = atag.getscreen(t)
        c:tags({ t })
    end
end

function ewmh.urgent(c, urgent)
    if c ~= client.focus and not aclient.property.get(c,"ignore_urgent") then
        c.urgent = urgent
    end
end

client.connect_signal("request::activate", ewmh.activate)
client.connect_signal("request::tag", ewmh.tag)
client.connect_signal("request::urgent", ewmh.urgent)
client.connect_signal("request::maximized_horizontal", maximized_horizontal)
client.connect_signal("request::maximized_vertical", maximized_vertical)
client.connect_signal("request::fullscreen", fullscreen)
client.connect_signal("property::screen", screen_change)
client.connect_signal("property::border_width", geometry_change)
client.connect_signal("property::geometry", geometry_change)

return ewmh

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
