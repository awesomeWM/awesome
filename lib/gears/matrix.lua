---------------------------------------------------------------------------
-- An implementation of matrices for describing and working with affine
-- transformations.
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
-- @classmod gears.matrix
---------------------------------------------------------------------------

local cairo = require("lgi").cairo
local matrix = {}

-- Metatable for matrix instances. This is set up near the end of the file.
local matrix_mt = {}

--- Create a new matrix instance
-- @tparam number xx The xx transformation part.
-- @tparam number yx The yx transformation part.
-- @tparam number xy The xy transformation part.
-- @tparam number yy The yy transformation part.
-- @tparam number x0 The x0 transformation part.
-- @tparam number y0 The y0 transformation part.
-- @return A new matrix describing the given transformation.
function matrix.create(xx, yx, xy, yy, x0, y0)
    return setmetatable({
        xx = xx, xy = xy, x0 = x0,
        yx = yx, yy = yy, y0 = y0
    }, matrix_mt)
end

--- Create a new translation matrix
-- @tparam number x The translation in x direction.
-- @tparam number y The translation in y direction.
-- @return A new matrix describing the given transformation.
function matrix.create_translate(x, y)
    return matrix.create(1, 0, 0, 1, x, y)
end

--- Create a new scaling matrix
-- @tparam number sx The scaling in x direction.
-- @tparam number sy The scaling in y direction.
-- @return A new matrix describing the given transformation.
function matrix.create_scale(sx, sy)
    return matrix.create(sx, 0, 0, sy, 0, 0)
end

--- Create a new rotation matrix
-- @tparam number angle The angle of the rotation in radians.
-- @return A new matrix describing the given transformation.
function matrix.create_rotate(angle)
    local c, s = math.cos(angle), math.sin(angle)
    return matrix.create(c, s, -s, c, 0, 0)
end

--- Create a new rotation matrix rotating around a custom point
-- @tparam number x The horizontal rotation point
-- @tparam number y The vertical rotation point
-- @tparam number angle The angle of the rotation in radians.
-- @return A new matrix describing the given transformation.
function matrix.create_rotate_at(x, y, angle)
    return   matrix.create_translate( -x, -y )
           * matrix.create_rotate   ( angle  )
           * matrix.create_translate(  x,  y )
end

--- Translate this matrix
-- @tparam number x The translation in x direction.
-- @tparam number y The translation in y direction.
-- @return A new matrix describing the new transformation.
function matrix:translate(x, y)
    return matrix.create_translate(x, y):multiply(self)
end

--- Scale this matrix
-- @tparam number sx The scaling in x direction.
-- @tparam number sy The scaling in y direction.
-- @return A new matrix describing the new transformation.
function matrix:scale(sx, sy)
    return matrix.create_scale(sx, sy):multiply(self)
end

--- Rotate this matrix
-- @tparam number angle The angle of the rotation in radians.
-- @return A new matrix describing the new transformation.
function matrix:rotate(angle)
    return matrix.create_rotate(angle):multiply(self)
end

--- Rotate a shape from a custom point
-- @tparam number x The horizontal rotation point
-- @tparam number y The vertical rotation point
-- @tparam number angle The angle (in radiant: -2*math.pi to 2*math.pi)
-- @return A transformation object
function matrix:rotate_at(x, y, angle)
    return self * matrix.create_rotate_at(x, y, angle)
end

--- Invert this matrix
-- @return A new matrix describing the inverse transformation.
function matrix:invert()
    -- Beware of math! (I just copied the algorithm from cairo's source code)
    local a, b, c, d, x0, y0 = self.xx, self.yx, self.xy, self.yy, self.x0, self.y0
    local inv_det = 1/(a*d - b*c)
    return matrix.create(inv_det * d, inv_det * -b,
            inv_det * -c, inv_det * a,
            inv_det * (c * y0 - d * x0), inv_det * (b * x0 - a * y0))
end

--- Multiply this matrix with another matrix.
-- The resulting matrix describes a transformation that is equivalent to first
-- applying this transformation and then the transformation from `other`.
-- Note that this function can also be called by directly multiplicating two
-- matrix instances: `a * b == a:multiply(b)`.
-- @tparam gears.matrix|cairo.Matrix other The other matrix to multiply with.
-- @return The multiplication result.
function matrix:multiply(other)
    local ret = matrix.create(self.xx * other.xx + self.yx * other.xy,
        self.xx * other.yx + self.yx * other.yy,
        self.xy * other.xx + self.yy * other.xy,
        self.xy * other.yx + self.yy * other.yy,
        self.x0 * other.xx + self.y0 * other.xy + other.x0,
        self.x0 * other.yx + self.y0 * other.yy + other.y0)

    return ret
end

--- Check if two matrices are equal.
-- Note that this function cal also be called by directly comparing two matrix
-- instances: `a == b`.
-- @tparam gears.matrix|cairo.Matrix other The matrix to compare with.
-- @return True if this and the other matrix are equal.
function matrix:equals(other)
    for _, k in pairs{ "xx", "xy", "yx", "yy", "x0", "y0" } do
        if self[k] ~= other[k] then
            return false
        end
    end
    return true
end

--- Get a string representation of this matrix
-- @return A string showing this matrix in column form.
function matrix:tostring()
    return string.format("[[%g, %g], [%g, %g], [%g, %g]]",
            self.xx, self.yx, self.xy,
            self.yy, self.x0, self.y0)
end

--- Transform a distance by this matrix.
-- The difference to @{matrix:transform_point} is that the translation part of
-- this matrix is ignored.
-- @tparam number x The x coordinate of the point.
-- @tparam number y The y coordinate of the point.
-- @treturn number The x coordinate of the transformed point.
-- @treturn number The x coordinate of the transformed point.
function matrix:transform_distance(x, y)
    return self.xx * x + self.xy * y, self.yx * x + self.yy * y
end

--- Transform a point by this matrix.
-- @tparam number x The x coordinate of the point.
-- @tparam number y The y coordinate of the point.
-- @treturn number The x coordinate of the transformed point.
-- @treturn number The y coordinate of the transformed point.
function matrix:transform_point(x, y)
    x, y = self:transform_distance(x, y)
    return self.x0 + x, self.y0 + y
end

--- Calculate a bounding rectangle for transforming a rectangle by a matrix.
-- @tparam number x The x coordinate of the rectangle.
-- @tparam number y The y coordinate of the rectangle.
-- @tparam number width The width of the rectangle.
-- @tparam number height The height of the rectangle.
-- @treturn number X coordinate of the bounding rectangle.
-- @treturn number Y coordinate of the bounding rectangle.
-- @treturn number Width of the bounding rectangle.
-- @treturn number Height of the bounding rectangle.
function matrix:transform_rectangle(x, y, width, height)
    -- Transform all four corners of the rectangle
    local x1, y1 = self:transform_point(x, y)
    local x2, y2 = self:transform_point(x, y + height)
    local x3, y3 = self:transform_point(x + width, y + height)
    local x4, y4 = self:transform_point(x + width, y)
    -- Find the extremal points of the result
    x = math.min(x1, x2, x3, x4)
    y = math.min(y1, y2, y3, y4)
    width = math.max(x1, x2, x3, x4) - x
    height = math.max(y1, y2, y3, y4) - y

    return x, y, width, height
end

--- Convert to a cairo matrix
-- @treturn cairo.Matrix A cairo matrix describing the same transformation.
function matrix:to_cairo_matrix()
    local ret = cairo.Matrix()
    ret:init(self.xx, self.yx, self.xy, self.yy, self.x0, self.y0)
    return ret
end

--- Convert to a cairo matrix
-- @tparam cairo.Matrix mat A cairo matrix describing the sought transformation
-- @treturn gears.matrix A matrix instance describing the same transformation.
function matrix.from_cairo_matrix(mat)
    return matrix.create(mat.xx, mat.yx, mat.xy, mat.yy, mat.x0, mat.y0)
end

matrix_mt.__index = matrix
matrix_mt.__newindex = error
matrix_mt.__eq = matrix.equals
matrix_mt.__mul = matrix.multiply
matrix_mt.__tostring = matrix.tostring

--- A constant for the identity matrix.
matrix.identity = matrix.create(1, 0, 0, 1, 0, 0)

return matrix

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
