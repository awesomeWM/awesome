---------------------------------------------------------------------------
--- Maximized and fullscreen layouts module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.layout.suit.max
---------------------------------------------------------------------------

-- Grab environment we need
local pairs = pairs
local client = require("awful.client")

local max = {}

local function fmax(p, fs)
    -- Fullscreen?
    local area
    if fs then
        area = p.geometry
    else
        area = p.workarea
    end

    for k, c in pairs(p.clients) do
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
-- @param screen The screen to arrange.
max.name = "max"
function max.arrange(p)
    return fmax(p, false)
end

--- Fullscreen layout.
-- @param screen The screen to arrange.
max.fullscreen = {}
max.fullscreen.name = "fullscreen"
function max.fullscreen.arrange(p)
    return fmax(p, true)
end

return max
