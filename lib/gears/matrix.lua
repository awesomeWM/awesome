---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @module gears.matrix
---------------------------------------------------------------------------

local cairo = require("lgi").cairo
local debug = require("gears.debug")
local matrix = {}

--- Copy a cairo matrix
-- @param matrix The matrix to copy.
-- @return A copy of the given cairo matrix.
function matrix.copy(matrix)
    debug.assert(cairo.Matrix:is_type_of(matrix), "Argument should be a cairo matrix")
    local ret = cairo.Matrix()
    ret:init(matrix.xx, matrix.yx, matrix.xy, matrix.yy, matrix.x0, matrix.y0)
    return ret
end

--- Check if two cairo matrices are equal
-- @param m1 The first matrix to compare with.
-- @param m2 The second matrix to compare with.
-- @return True if they are equal.
function matrix.equals(m1, m2)
    for _, k in pairs{ "xx", "xy", "yx", "yy", "x0", "y0" } do
        if m1[k] ~= m2[k] then
            return false
        end
    end
    return true
end

--- Calculate a bounding rectangle for transforming a rectangle by a matrix.
-- @param matrix The cairo matrix that describes the transformation.
-- @param x The x coordinate of the rectangle.
-- @param y The y coordinate of the rectangle.
-- @param width The width of the rectangle.
-- @param height The height of the rectangle.
-- @return The x, y, width and height of the bounding rectangle.
function matrix.transform_rectangle(matrix, x, y, width, height)
    -- Transform all four corners of the rectangle
    local x1, y1 = matrix:transform_point(x, y)
    local x2, y2 = matrix:transform_point(x, y + height)
    local x3, y3 = matrix:transform_point(x + width, y + height)
    local x4, y4 = matrix:transform_point(x + width, y)
    -- Find the extremal points of the result
    local x = math.min(x1, x2, x3, x4)
    local y = math.min(y1, y2, y3, y4)
    local width = math.max(x1, x2, x3, x4) - x
    local height = math.max(y1, y2, y3, y4) - y

    return x, y, width, height
end

return matrix

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
