---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local ipairs = ipairs
local pairs = pairs
local math = math
local table = table
local capi =
{
    screen = screen,
    mouse = mouse,
    client = client
}
local client = require("awful.client")
local layout = require("awful.layout")
local a_screen = require("awful.screen")

--- Places client according to special criteria.
-- awful.placement
local placement = {}

-- Check if an area intersect another area.
-- @param a The area.
-- @param b The other area.
-- @return True if they intersect, false otherwise.
local function area_intersect_area(a, b)
    return (b.x < a.x + a.width
            and b.x + b.width > a.x
            and b.y < a.y + a.height
            and b.y + b.height > a.y)
end

-- Get the intersect area between a and b.
-- @param a The area.
-- @param b The other area.
-- @return The intersect area.
local function area_intersect_area_get(a, b)
    local g = {}
    g.x = math.max(a.x, b.x)
    g.y = math.max(a.y, b.y)
    g.width = math.min(a.x + a.width, b.x + b.width) - g.x
    g.height = math.min(a.y + a.height, b.y + b.height) - g.y
    return g
end

-- Remove an area from a list, splitting the space between several area that
-- can overlap.
-- @param areas Table of areas.
-- @param elem Area to remove.
-- @return The new area list.
local function area_remove(areas, elem)
    for i = #areas, 1, -1 do
        -- Check if the 'elem' intersect
        if area_intersect_area(areas[i], elem) then
            -- It does? remove it
            local r = table.remove(areas, i)
            local inter = area_intersect_area_get(r, elem)

            if inter.x > r.x then
                table.insert(areas, {
                    x = r.x,
                    y = r.y,
                    width = inter.x - r.x,
                    height = r.height
                })
            end

            if inter.y > r.y then
                table.insert(areas, {
                    x = r.x,
                    y = r.y,
                    width = r.width,
                    height = inter.y - r.y
                })
            end

            if inter.x + inter.width < r.x + r.width then
                table.insert(areas, {
                    x = inter.x + inter.width,
                    y = r.y,
                    width = (r.x + r.width) - (inter.x + inter.width),
                    height = r.height
                })
            end

            if inter.y + inter.height < r.y + r.height then
                table.insert(areas, {
                    x = r.x,
                    y = inter.y + inter.height,
                    width = r.width,
                    height = (r.y + r.height) - (inter.y + inter.height)
                })
            end
        end
    end

    return areas
end

--- Place the client so no part of it will be outside the screen.
-- @param c The client.
-- @return The new client geometry.
function placement.no_offscreen(c)
    local c = c or capi.client.focus
    local geometry = c:geometry()
    local screen   = c.screen or a_screen.getbycoord(geometry.x, geometry.y)
    local border   = c.border_width
    local screen_geometry = capi.screen[screen].workarea

    if geometry.x + geometry.width + 2*border > screen_geometry.x + screen_geometry.width then
        geometry.x = screen_geometry.x + screen_geometry.width - geometry.width - 2*border
    elseif geometry.x < screen_geometry.x then
        geometry.x = screen_geometry.x
    end

    if geometry.y + geometry.height + border > screen_geometry.y + screen_geometry.height then
        geometry.y = screen_geometry.y + screen_geometry.height - geometry.height - 2*border
    elseif geometry.y < screen_geometry.y then
        geometry.y = screen_geometry.y
    end

    c:geometry(geometry)
end

--- Place the client where there's place available with minimum overlap.
-- @param c The client.
function placement.no_overlap(c)
    local geometry = c:geometry()
    local screen   = c.screen or a_screen.getbycoord(geometry.x, geometry.y)
    local cls = client.visible(screen)
    local curlay = layout.get()
    local areas = { capi.screen[screen].workarea }
    for i, cl in pairs(cls) do
        if cl ~= c and cl.type ~= "desktop" and (client.floating.get(cl) or curlay == layout.suit.floating) then
            areas = area_remove(areas, cl:geometry())
        end
    end

    -- Look for available space
    local found = false
    local new = { x = geometry.x, y = geometry.y, width = 0, height = 0 }
    for i, r in ipairs(areas) do
        if r.width >= geometry.width
           and r.height >= geometry.height
           and r.width * r.height > new.width * new.height then
            found = true
            new = r
            -- Check if the client's current position is available
            -- and prefer that one (why move it around pointlessly?)
            if     geometry.x >= r.x
               and geometry.y >= r.y
               and geometry.x + geometry.width <= r.x + r.width
               and geometry.y + geometry.height <= r.y + r.height then
                new.x = geometry.x
                new.y = geometry.y
            end
        end
    end

    -- We did not find an area with enough space for our size:
    -- just take the biggest available one and go in
    if not found then
        for i, r in ipairs(areas) do
            if r.width * r.height > new.width * new.height then
                new = r
            end
        end
    end

    -- Restore height and width
    new.width = geometry.width
    new.height = geometry.height

    return c:geometry(new)
end

--- Place the client under the mouse.
-- @param c The client.
-- @return The new client geometry.
function placement.under_mouse(c)
    local c = c or capi.client.focus
    local c_geometry = c:geometry()
    local m_coords = capi.mouse.coords()
    return c:geometry({ x = m_coords.x - c_geometry.width / 2,
                        y = m_coords.y - c_geometry.height / 2 })
end

--- Place the client centered with respect to a parent or the clients screen.
-- @param c The client.
-- @param p The parent (optional, nil for screen centering).
-- @return The new client geometry.
function placement.centered(c, p)
    local c = c or capi.client.focus
    local c_geometry = c:geometry()
    local screen = c.screen or a_screen.getbycoord(c_geometry.x, c_geometry.y)
    local s_geometry
    if p then
        s_geometry = p:geometry()
    else
        s_geometry = capi.screen[screen].geometry
    end
    return c:geometry({ x = s_geometry.x + (s_geometry.width - c_geometry.width) / 2,
                        y = s_geometry.y + (s_geometry.height - c_geometry.height) / 2 })
end

--- Place the client centered on the horizontal axis with respect to a parent or the clients screen.
-- @param c The client.
-- @param p The parent (optional, nil for screen centering).
-- @return The new client geometry.
function placement.center_horizontal(c, p)
    local c = c or capi.client.focus
    local c_geometry = c:geometry()
    local screen = c.screen or a_screen.getbycoord(c_geometry.x, c_geometry.y)
    local s_geometry
    if p then
        s_geometry = p:geometry()
    else
        s_geometry = capi.screen[screen].geometry
    end
    return c:geometry({ x = s_geometry.x + (s_geometry.width - c_geometry.width) / 2 })
end

--- Place the client centered on the vertical axis with respect to a parent or the clients screen.
-- @param c The client.
-- @param p The parent (optional, nil for screen centering).
-- @return The new client geometry.
function placement.center_vertical(c, p)
    local c = c or capi.client.focus
    local c_geometry = c:geometry()
    local screen = c.screen or a_screen.getbycoord(c_geometry.x, c_geometry.y)
    local s_geometry
    if p then
        s_geometry = p:geometry()
    else
        s_geometry = capi.screen[screen].geometry
    end
    return c:geometry({ y = s_geometry.y + (s_geometry.height - c_geometry.height) / 2 })
end

return placement

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
