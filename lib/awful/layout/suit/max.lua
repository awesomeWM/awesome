---------------------------------------------------------------------------
--- Maximized and fullscreen layouts module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module awful.layout
---------------------------------------------------------------------------

-- Grab environment we need
local pairs = pairs

local max = {}

--- The max layout layoutbox icon.
-- @beautiful beautiful.layout_max
-- @param surface
-- @see gears.surface

--- The fullscreen layout layoutbox icon.
-- @beautiful beautiful.layout_fullscreen
-- @param surface
-- @see gears.surface

local function fmax(p, fs)
    -- Fullscreen?
    local area
    if fs then
        area = p.geometry
    else
        area = p.workarea
    end

    for _, c in pairs(p.clients) do
        local g = {
            x = area.x,
            y = area.y,
            width = area.width,
            height = area.height
        }
        p.geometries[c] = g
    end
end

--- Maximized layout.
-- @clientlayout awful.layout.suit.max.name
max.name = "max"
function max.arrange(p)
    return fmax(p, false)
end
function max.skip_gap(nclients, t) -- luacheck: no unused args
    return true
end

--- Fullscreen layout.
-- @clientlayout awful.layout.suit.max.fullscreen
max.fullscreen = {}
max.fullscreen.name = "fullscreen"
max.fullscreen.skip_gap = max.skip_gap
function max.fullscreen.arrange(p)
    return fmax(p, true)
end

return max

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
