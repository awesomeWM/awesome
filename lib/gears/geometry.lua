---------------------------------------------------------------------------
--
-- Helper functions used to compute geometries.
--
-- When this module refer to a geometry table, this assume a table with at least
-- an *x*, *y*, *width* and *height* keys and numeric values.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
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

return gears.geometry
