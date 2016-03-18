---------------------------------------------------------------------------
--- Places client according to special criteria.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.placement
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
local dpi = require("beautiful").xresources.apply_dpi

local function get_screen(s)
    return s and capi.screen[s]
end

local placement = {}

-- 3x3 matrix of the valid sides and corners
local corners3x3 = {{"top_left"   ,   "top"   , "top_right"   },
                    {"left"       ,    nil    , "right"       },
                    {"bottom_left",  "bottom" , "bottom_right"}}

-- 2x2 matrix of the valid sides and corners
local corners2x2 = {{"top_left"   ,            "top_right"   },
                    {"bottom_left",            "bottom_right"}}

--- Get the area covered by a drawin.
-- @param d The drawin
-- @tparam[opt=nil] table new_geo A new geometry
-- @tparam[opt=false] boolean ignore_border_width Ignore the border
-- @treturn The drawin's area.
local function area_common(d, new_geo, ignore_border_width)
    -- The C side expect no arguments, nil isn't valid
    local geometry = new_geo and d:geometry(new_geo) or d:geometry()
    local border = ignore_border_width and 0 or d.border_width or 0
    geometry.x = geometry.x - border
    geometry.y = geometry.y - border
    geometry.width = geometry.width + 2 * border
    geometry.height = geometry.height + 2 * border
    return geometry
end

--- Get (and optionally set) an object geometry.
-- Some elements, such as `mouse` and `screen` don't have a `:geometry()`
-- methods.
-- @param obj An object
-- @tparam table args the method arguments
-- @tparam[opt=nil] table new_geo A new geometry to replace the existing one
-- @tparam[opt=false] boolean ignore_border_width Ignore the border
-- @treturn table A table with *x*, *y*, *width* and *height*.
local function geometry_common(obj, args, new_geo, ignore_border_width)
    -- It's a mouse
    if obj.coords then
        local coords = new_geo and obj.coords(new_geo) or obj.coords()
        return {x=coords.x, y=coords.y, width=0, height=0}
    elseif obj.geometry then
        local geo = obj.geometry

        -- It is either a drawable or something that implement its API
        if type(geo) == "function" then
            local dgeo = area_common(obj, new_geo, ignore_border_width)

            -- Apply the margins
            if args.margins then
                local delta = type(args.margins) == "table" and args.margins or {
                    left = args.margins , right  = args.margins,
                    top  = args.margins , bottom = args.margins
                }

                return {
                    x      = dgeo.x      + (delta.left or 0),
                    y      = dgeo.y      + (delta.top  or 0),
                    width  = dgeo.width  - (delta.left or 0) - (delta.right  or 0),
                    height = dgeo.height - (delta.top  or 0) - (delta.bottom or 0),
                }
            end

            return dgeo
        end

        -- It is a screen, it doesn't support setting new sizes.
        return a_screen.get_bounding_geometry(obj, args)
    else
        assert(false, "Invalid object")
    end
end

--- Get the parent geometry from the standardized arguments API shared by all
-- `awful.placement` methods.
-- @param obj A screen or a drawable
-- @tparam table args the method arguments
-- @treturn table A table with *x*, *y*, *width* and *height*.
local function get_parent_geometry(obj, args)
    if args.bounding_rect then
        return args.bounding_rect
    elseif args.parent then
        return geometry_common(args.parent, args)
    elseif obj.screen then
        return geometry_common(obj.screen, args)
    else
        return geometry_common(capi.screen[capi.mouse.screen], args)
    end
end

--- Convert a rectangle and matrix coordinates info into a point.
-- This is useful along with matrixes like `corners3x3` to convert
-- indices into a geometry point.
-- @tparam table geo a geometry table
-- @tparam number corner_i The horizontal matrix index
-- @tparam number corner_j The vertical matrix index
-- @tparam number n The (square) matrix dimension
-- @treturn table A table with *x* and *y* keys
local function rect_to_point(geo, corner_i, corner_j, n)
    return {
        x = geo.x + corner_i * math.floor(geo.width  / (n-1)),
        y = geo.y + corner_j * math.floor(geo.height / (n-1)),
    }
end

--- Move a point into an area.
-- This doesn't change the *width* and *height* values, allowing the target
-- area to be smaller than the source one.
-- @tparam table source The (larger) geometry to move `target` into
-- @tparam table target The area to move into `source`
-- @treturn table A table with *x* and *y* keys
local function move_into_geometry(source, target)
    local ret = {x = target.x, y = target.y}

    -- Horizontally
    if ret.x < source.x then
        ret.x = source.x
    elseif ret.x > source.x + source.width then
        ret.x = source.x + source.width - 1
    end

    -- Vertically
    if ret.y < source.y then
        ret.y = source.y
    elseif ret.y > source.y + source.height then
        ret.y = source.y + source.height - 1
    end

    return ret
end

--- Check if an area intersect another area.
-- @param a The area.
-- @param b The other area.
-- @return True if they intersect, false otherwise.
local function area_intersect_area(a, b)
    return (b.x < a.x + a.width
            and b.x + b.width > a.x
            and b.y < a.y + a.height
            and b.y + b.height > a.y)
end

--- Get the intersect area between a and b.
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

--- Remove an area from a list, splitting the space between several area that
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

--- Move a drawable to the closest corner of the parent geometry (such as the
-- screen).
--
-- Valid arguments include the common ones and:
--
-- * **include_sides**: Also include the left, right, top and bottom positions
--
--@DOC_awful_placement_closest_mouse_EXAMPLE@
-- @tparam[opt=client.focus] drawable d A drawable (like `client`, `mouse`
--   or `wibox`)
-- @tparam[opt={}] table args The arguments
-- @treturn string The corner name
function placement.closest_corner(d, args)
    d = d or capi.client.focus

    local sgeo = get_parent_geometry(d, args)
    local dgeo = geometry_common(d, args)

    local pos  = move_into_geometry(sgeo, dgeo)

    local corner_i, corner_j, n

    -- Use the product of 3 to get the closest point in a NxN matrix
    local function f(_n, mat)
        n        = _n
        corner_i = -math.ceil( ( (sgeo.x - pos.x) * n) / sgeo.width  )
        corner_j = -math.ceil( ( (sgeo.y - pos.y) * n) / sgeo.height )
        return mat[corner_j + 1][corner_i + 1]
    end

    -- Turn the area into a grid and snap to the cloest point. This size of the
    -- grid will increase the accuracy. A 2x2 matrix only include the corners,
    -- at 3x3, this include the sides too technically, a random size would work,
    -- but without corner names.
    local grid_size = args.include_sides and 3 or 2

    -- If the point is in the center, use the closest corner
    local corner = f(grid_size, corners3x3) or f(2, corners2x2)

    -- Transpose the corner back to the original size
    local new_args = setmetatable({position = corner}, {__index=args})
    geometry_common(d, new_args, rect_to_point(dgeo, corner_i, corner_j , n))

    return corner
end

--- Place the client so no part of it will be outside the screen (workarea).
-- @client c The client.
-- @tparam[opt=client's screen] integer screen The screen.
-- @treturn table The new client geometry.
function placement.no_offscreen(c, screen)
    c = c or capi.client.focus
    local geometry = area_common(c)
    screen = get_screen(screen or c.screen or a_screen.getbycoord(geometry.x, geometry.y))
    local screen_geometry = screen.workarea

    if geometry.x + geometry.width > screen_geometry.x + screen_geometry.width then
        geometry.x = screen_geometry.x + screen_geometry.width - geometry.width
    end
    if geometry.x < screen_geometry.x then
        geometry.x = screen_geometry.x
    end

    if geometry.y + geometry.height > screen_geometry.y + screen_geometry.height then
        geometry.y = screen_geometry.y + screen_geometry.height - geometry.height
    end
    if geometry.y < screen_geometry.y then
        geometry.y = screen_geometry.y
    end

    return c:geometry({ x = geometry.x, y = geometry.y })
end

--- Place the client where there's place available with minimum overlap.
-- @param c The client.
function placement.no_overlap(c)
    local geometry = area_common(c)
    local screen   = get_screen(c.screen or a_screen.getbycoord(geometry.x, geometry.y))
    local cls = client.visible(screen)
    local curlay = layout.get()
    local areas = { screen.workarea }
    for _, cl in pairs(cls) do
        if cl ~= c and cl.type ~= "desktop" and (client.floating.get(cl) or curlay == layout.suit.floating) then
            areas = area_remove(areas, area_common(cl))
        end
    end

    -- Look for available space
    local found = false
    local new = { x = geometry.x, y = geometry.y, width = 0, height = 0 }
    for _, r in ipairs(areas) do
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
        for _, r in ipairs(areas) do
            if r.width * r.height > new.width * new.height then
                new = r
            end
        end
    end

    -- Restore height and width
    new.width = geometry.width
    new.height = geometry.height

    return c:geometry({ x = new.x, y = new.y })
end

--- Place the client under the mouse.
-- @param c The client.
-- @return The new client geometry.
function placement.under_mouse(c)
    c = c or capi.client.focus
    local c_geometry = area_common(c)
    local m_coords = capi.mouse.coords()
    return c:geometry({ x = m_coords.x - c_geometry.width / 2,
                        y = m_coords.y - c_geometry.height / 2 })
end

--- Place the client next to the mouse.
--
-- It will place `c` next to the mouse pointer, trying the following positions
-- in this order: right, left, above and below.
-- @client[opt=focused] c The client.
-- @tparam[opt=apply_dpi(5)] integer offset The offset from the mouse position.
-- @return The new client geometry.
function placement.next_to_mouse(c, offset)
    c = c or capi.client.focus
    offset = offset or dpi(5)
    local c_geometry = area_common(c)
    local c_width = c_geometry.width
    local c_height = c_geometry.height
    local m_coords = capi.mouse.coords()
    local screen_geometry = capi.screen[capi.mouse.screen].workarea

    local x, y

    -- Prefer it to be on the right.
    x = m_coords.x + offset
    if x + c_width > screen_geometry.width then
        -- Then to the left.
        x = m_coords.x - c_width - offset
    end
    if x < screen_geometry.x then
        -- Then above.
        x = m_coords.x - math.ceil(c_width / 2)
        y = m_coords.y - c_height - offset
        if y < screen_geometry.y then
            -- Finally below.
            y = m_coords.y + offset
        end
    else
        y = m_coords.y - math.ceil(c_height / 2)
    end
    return c:geometry({ x = x, y = y })
end

--- Place the client centered with respect to a parent or the clients screen.
-- @param c The client.
-- @param[opt] p The parent (nil for screen centering).
-- @return The new client geometry.
function placement.centered(c, p)
    c = c or capi.client.focus
    local c_geometry = area_common(c)
    local screen = get_screen(c.screen or a_screen.getbycoord(c_geometry.x, c_geometry.y))
    local s_geometry
    if p then
        s_geometry = area_common(p)
    else
        s_geometry = screen.geometry
    end
    return c:geometry({ x = s_geometry.x + (s_geometry.width - c_geometry.width) / 2,
                        y = s_geometry.y + (s_geometry.height - c_geometry.height) / 2 })
end

--- Place the client centered on the horizontal axis with respect to a parent or the clients screen.
-- @param c The client.
-- @param[opt] p The parent (nil for screen centering).
-- @return The new client geometry.
function placement.center_horizontal(c, p)
    c = c or capi.client.focus
    local c_geometry = area_common(c)
    local screen = get_screen(c.screen or a_screen.getbycoord(c_geometry.x, c_geometry.y))
    local s_geometry
    if p then
        s_geometry = area_common(p)
    else
        s_geometry = screen.geometry
    end
    return c:geometry({ x = s_geometry.x + (s_geometry.width - c_geometry.width) / 2 })
end

--- Place the client centered on the vertical axis with respect to a parent or the clients screen.
-- @param c The client.
-- @param[opt] p The parent (nil for screen centering).
-- @return The new client geometry.
function placement.center_vertical(c, p)
    c = c or capi.client.focus
    local c_geometry = area_common(c)
    local screen = get_screen(c.screen or a_screen.getbycoord(c_geometry.x, c_geometry.y))
    local s_geometry
    if p then
        s_geometry = area_common(p)
    else
        s_geometry = screen.geometry
    end
    return c:geometry({ y = s_geometry.y + (s_geometry.height - c_geometry.height) / 2 })
end

return placement

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
