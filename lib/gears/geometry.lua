---------------------------------------------------------------------------
--
-- Helper functions used to compute geometries.
--
-- When this module refer to a geometry table, this assume a table with at least
-- an *x*, *y*, *width* and *height* keys and numeric values.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module gears.geometry
---------------------------------------------------------------------------
local math = math

local gears = {geometry = {rectangle = {} } }

--- Get the square distance between a rectangle and a point.
-- @tparam table geom A rectangle
-- @tparam number geom.x The horizontal coordinate
-- @tparam number geom.y The vertical coordinate
-- @tparam number geom.width The rectangle width
-- @tparam number geom.height The rectangle height
-- @tparam number x X coordinate of point
-- @tparam number y Y coordinate of point
-- @treturn number The squared distance of the rectangle to the provided point
function gears.geometry.rectangle.get_square_distance(geom, x, y)
    local dist_x, dist_y = 0, 0
    if x < geom.x then
        dist_x = geom.x - x
    elseif x >= geom.x + geom.width then
        dist_x = x - geom.x - geom.width + 1
    end
    if y < geom.y then
        dist_y = geom.y - y
    elseif y >= geom.y + geom.height then
        dist_y = y - geom.y - geom.height + 1
    end
    return dist_x * dist_x + dist_y * dist_y
end

--- Return the closest rectangle from `list` for a given point.
-- @tparam table list A list of geometry tables.
-- @tparam number x The x coordinate
-- @tparam number y The y coordinate
-- @return The key from the closest geometry.
function gears.geometry.rectangle.get_closest_by_coord(list, x, y)
    local dist = math.huge
    local ret = nil

    for k, v in pairs(list) do
        local d = gears.geometry.rectangle.get_square_distance(v, x, y)
        if d < dist then
            ret, dist = k, d
        end
    end

    return ret
end

--- Return the rectangle containing the [x, y] point.
--
-- Note that if multiple element from the geometry list contains the point, the
-- returned result is nondeterministic.
--
-- @tparam table list A list of geometry tables.
-- @tparam number x The x coordinate
-- @tparam number y The y coordinate
-- @return The key from the closest geometry. In case no result is found, *nil*
--  is returned.
function gears.geometry.rectangle.get_by_coord(list, x, y)
    for k, geometry in pairs(list) do
        if x >= geometry.x and x < geometry.x + geometry.width
           and y >= geometry.y and y < geometry.y + geometry.height then
            return k
        end
    end
end

--- Return true whether rectangle B is in the right direction
-- compared to rectangle A.
-- @param dir The direction.
-- @param gA The geometric specification for rectangle A.
-- @param gB The geometric specification for rectangle B.
-- @return True if B is in the direction of A.
local function is_in_direction(dir, gA, gB)
    if dir == "up" then
        return gA.y > gB.y
    elseif dir == "down" then
        return gA.y < gB.y
    elseif dir == "left" then
        return gA.x > gB.x
    elseif dir == "right" then
        return gA.x < gB.x
    end
    return false
end

--- Calculate distance between two points.
-- i.e: if we want to move to the right, we will take the right border
-- of the currently focused screen and the left side of the checked screen.
-- @param dir The direction.
-- @param _gA The first rectangle.
-- @param _gB The second rectangle.
-- @return The distance between the screens.
local function calculate_distance(dir, _gA, _gB)
    local gAx = _gA.x
    local gAy = _gA.y
    local gBx = _gB.x
    local gBy = _gB.y

    if dir == "up" then
        gBy = _gB.y + _gB.height
    elseif dir == "down" then
        gAy = _gA.y + _gA.height
    elseif dir == "left" then
        gBx = _gB.x + _gB.width
    elseif dir == "right" then
        gAx = _gA.x + _gA.width
    end

    return math.sqrt((gBx - gAx) ^ 2 + (gBy - gAy) ^ 2)
end

--- Get the nearest rectangle in the given direction. Every rectangle is specified as a table
-- with *x*, *y*, *width*, *height* keys, the same as client or screen geometries.
-- @tparam string dir The direction, can be either *up*, *down*, *left* or *right*.
-- @tparam table recttbl A table of rectangle specifications.
-- @tparam table cur The current rectangle.
-- @return The index for the rectangle in recttbl closer to cur in the given direction. nil if none found.
function gears.geometry.rectangle.get_in_direction(dir, recttbl, cur)
    local dist, dist_min
    local target = nil

    -- We check each object
    for i, rect in pairs(recttbl) do
        -- Check geometry to see if object is located in the right direction.
        if is_in_direction(dir, cur, rect) then
            -- Calculate distance between current and checked object.
            dist = calculate_distance(dir, cur, rect)

            -- If distance is shorter then keep the object.
            if not target or dist < dist_min then
                target = i
                dist_min = dist
            end
        end
    end
    return target
end

--- Check if an area intersect another area.
-- @param a The area.
-- @param b The other area.
-- @return True if they intersect, false otherwise.
function gears.geometry.rectangle.area_intersect_area(a, b)
    return (b.x < a.x + a.width
            and b.x + b.width > a.x
            and b.y < a.y + a.height
            and b.y + b.height > a.y)
end

--- Get the intersect area between a and b.
-- @tparam table a The area.
-- @tparam number a.x The horizontal coordinate
-- @tparam number a.y The vertical coordinate
-- @tparam number a.width The rectangle width
-- @tparam number a.height The rectangle height
-- @tparam table b The other area.
-- @tparam number b.x The horizontal coordinate
-- @tparam number b.y The vertical coordinate
-- @tparam number b.width The rectangle width
-- @tparam number b.height The rectangle height
-- @treturn table The intersect area.
function gears.geometry.rectangle.get_intersection(a, b)
    local g = {}
    g.x = math.max(a.x, b.x)
    g.y = math.max(a.y, b.y)
    g.width = math.min(a.x + a.width, b.x + b.width) - g.x
    g.height = math.min(a.y + a.height, b.y + b.height) - g.y
    if g.width <= 0 or g.height <= 0 then
        g.width, g.height = 0, 0
    end
    return g
end

--- Remove an area from a list, splitting the space between several area that
-- can overlap.
-- @tparam table areas Table of areas.
-- @tparam table elem Area to remove.
-- @tparam number elem.x The horizontal coordinate
-- @tparam number elem.y The vertical coordinate
-- @tparam number elem.width The rectangle width
-- @tparam number elem.height The rectangle height
-- @return The new area list.
function gears.geometry.rectangle.area_remove(areas, elem)
    for i = #areas, 1, -1 do
        -- Check if the 'elem' intersect
        if gears.geometry.rectangle.area_intersect_area(areas[i], elem) then
            -- It does? remove it
            local r = table.remove(areas, i)
            local inter = gears.geometry.rectangle.get_intersection(r, elem)

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

return gears.geometry

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
