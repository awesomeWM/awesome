---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local setmetatable = setmetatable
local client = client
local screen = screen
local ipairs = ipairs
local math = math
local atag = require("awful.tag")

--- Implements EWMH requests handling.
-- awful.ewmh
local ewmh = {}

local data = setmetatable({}, { __mode = 'k' })

local function store_geometry(window, reqtype)
    if not data[window] then data[window] = {} end
    if not data[window][reqtype] then data[window][reqtype] = {} end
    data[window][reqtype] = window:geometry()
    data[window][reqtype].screen = window.screen
end

-- Maximize a window horizontally.
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

-- Maximize a window vertically.
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

-- Fullscreen a window.
-- @param window The window.
-- @param set Set or unset the fullscreen values.
local function fullscreen(window, set)
    if set then
        store_geometry(window, "fullscreen")
        data[window].fullscreen.border_width = window.border_width
        local g = screen[window.screen].geometry
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
                        data[window][reqtype].x = new_x
                    end
                    if data[window][reqtype].y then
                        new_y = to.y + data[window][reqtype].y - from.y
                        if new_y > to.y + to.width then new_y = to.y end
                        data[window][reqtype].y = new_y
                    end
                end
            end
        end
    end
end

-- Update a client's settings when its border width changes
local function border_change(window)
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
        -- This *will* cause an endless loop if some other property::border_width
        -- signal dares to change the border width, too, so don't do that!
        window.border_width = 0
        window:geometry(screen[window.screen].geometry)
    end
end

-- Activate a window
function ewmh.activate(c)
    if c:isvisible() then
        client.focus = c
        c:raise()
    elseif not awesome.startup then
        c.urgent = true
    end
end

-- Tag a window with its requested tag
function ewmh.tag(c, t)
    if not t then
        c.sticky = true
    else
        c.screen = atag.getscreen(t)
        c:tags({ t })
    end
end

client.connect_signal("request::activate", ewmh.activate)
client.connect_signal("request::tag", ewmh.tag)
client.connect_signal("request::maximized_horizontal", maximized_horizontal)
client.connect_signal("request::maximized_vertical", maximized_vertical)
client.connect_signal("request::fullscreen", fullscreen)
client.connect_signal("property::screen", screen_change)
client.connect_signal("property::border_width", border_change)

return ewmh

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
