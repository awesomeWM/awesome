---------------------------------------------------------------------------
--- Dwindle and spiral layouts
--
-- @author Uli Schlachter &lt;psychon@znc.in&gt;
-- @copyright 2009 Uli Schlachter
-- @copyright 2008 Julien Danjou
--
-- @module awful.layout
---------------------------------------------------------------------------

-- Grab environment we need
local ipairs = ipairs
local math = math

--- The spiral layout layoutbox icon.
-- @beautiful beautiful.layout_spiral
-- @param surface
-- @see gears.surface

--- The dwindle layout layoutbox icon.
-- @beautiful beautiful.layout_dwindle
-- @param surface
-- @see gears.surface

local spiral = {}

local function do_spiral(p, _spiral)
    local wa = p.workarea
    local cls = p.clients
    local n = #cls
    local old_width, old_height = wa.width, 2 * wa.height

    for k, c in ipairs(cls) do
        if k % 2 == 0 then
            wa.width, old_width = math.ceil(old_width / 2), wa.width
            if k ~= n then
                wa.height, old_height = math.floor(wa.height / 2), wa.height
            end
        else
            wa.height, old_height = math.ceil(old_height / 2), wa.height
            if k ~= n then
                wa.width, old_width = math.floor(wa.width / 2), wa.width
            end
        end

        if k % 4 == 0 and _spiral then
            wa.x = wa.x - wa.width
        elseif k % 2 == 0 then
            wa.x = wa.x + old_width
        elseif k % 4 == 3 and k < n and _spiral then
            wa.x = wa.x + math.ceil(old_width / 2)
        end

        if k % 4 == 1 and k ~= 1 and _spiral then
            wa.y = wa.y - wa.height
        elseif k % 2 == 1 and k ~= 1 then
            wa.y = wa.y + old_height
        elseif k % 4 == 0 and k < n and _spiral then
            wa.y = wa.y + math.ceil(old_height / 2)
        end

        local g = {
            x = wa.x,
            y = wa.y,
            width = wa.width,
            height = wa.height
        }
        p.geometries[c] = g
    end
end

--- Dwindle layout.
-- @clientlayout awful.layout.suit.spiral.dwindle
spiral.dwindle = {}
spiral.dwindle.name = "dwindle"
function spiral.dwindle.arrange(p)
    return do_spiral(p, false)
end

--- Spiral layout.
-- @clientlayout awful.layout.suit.spiral.name
spiral.name = "spiral"
function spiral.arrange(p)
    return do_spiral(p, true)
end

return spiral

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
