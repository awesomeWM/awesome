---------------------------------------------------------------------------
--- Module dedicated to gather common shape painters.
--
-- It add the concept of "shape" to Awesome. A shape can be applied to a
-- background, a margin, a mask or a drawable shape bounding.
--
-- The functions exposed by this module always take a context as first
-- parameter followed by the widget and height and additional parameters.
--
-- The functions provided by this module only create a path in the content.
-- to actually draw the content, use cr:fill(), cr:mask(), cr:slip() or
-- cr:stroke()
--
-- In many case, it is necessary to apply the shape using a transformation
-- such as a rotation. The preferred way to do this is to wrap the function
-- in another function calling `cr:rotate()` (or any other transformation
-- matrix)
--
-- @author Emmanuel Lepage Vallee
-- @copyright 2011-2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module gears.shape
---------------------------------------------------------------------------

local module = {}

local cairo = require( "lgi" ).cairo

--- Add a rounded rectangle to the current path
-- @note If the radius is bigger than either half side, it will be reduced
-- @param cr A cairo content
-- @param width The rectangle width
-- @param height The rectangle height
-- @param radius the corner radius
function module.rounded_rect(cr, width, height, radius)
    if width / 2 < radius then
        radius = width / 2
    end

    if height / 2 < radius then
        radius = height / 2
    end

    cr:arc( radius      , radius       , radius,    math.pi   , 3*(math.pi/2) )
    cr:arc( width-radius, radius       , radius, 3*(math.pi/2),    math.pi*2  )
    cr:arc( width-radius, height-radius, radius,    math.pi*2 ,    math.pi/2  )
    cr:arc( radius      , height-radius, radius,    math.pi/2 ,    math.pi    )

    cr:close_path()
end

--- Add a rectangle delimited by 2 180 degree arcs to the path
-- @param cr A cairo content
-- @param width The rectangle width
-- @param height The rectangle height
function module.rounded_bar(cr, width, height)
    module.rounded_rect(cr, width, height, height / 2)
end

-- Create a transformation matrix a*b
local function multiply(a, b)
    local m = cairo.Matrix()
    m:multiply(a, b)
    return m
end

local transformations = {}

--- Translate a shape
-- @param self A transformation object
-- @tparam number x The horizontal translation
-- @tparam number y The vertical translation
-- @return A transformation object
function transformations.translate(self, x, y)
    local m = cairo.Matrix()
    cairo.Matrix.init_translate(m, x, y)
    self.matrix = self.matrix and multiply(self.matrix, m) or m
    return self
end

--- Scale a shape
-- @param self A transformation object
-- @tparam number x The horizontal scale factor
-- @tparam number y The vertical scale factor
-- @return A transformation object
function transformations.scale(self, x, y)
    local m = cairo.Matrix()
    cairo.Matrix.init_scale(m, x, y)
    self.matrix = self.matrix and multiply(self.matrix, m) or m
    return self
end

--- Rotate a shape from the top left corner
-- @param self A transformation object
-- @tparam number radiant The angle (-2*math.pi to 2*math.pi)
-- @return A transformation object
function transformations.rotate(self, radiant)
    local m = cairo.Matrix()
    cairo.Matrix.init_rotate(m, radiant)
    self.matrix = self.matrix and multiply(self.matrix, m) or m
    return self
end

--- Rotate a shape from a custom point
-- @param self A transformation object
-- @tparam number x The horizontal rotation point
-- @tparam number y The vertical rotation point
-- @tparam number radiant The angle (-2*math.pi to 2*math.pi)
-- @return A transformation object
function transformations.rotate_at(self, x, y, radiant)
    transformations.translate(self, -x, -y)
    local m = cairo.Matrix()
    cairo.Matrix.init_rotate(m, radiant)
    self.matrix = self.matrix and multiply(self.matrix, m) or m
    transformations.translate(self, x, y)
    return self
end

--- Ajust the shape using a transformation object
--
-- Apply various transformations to the shape
--
-- @usage gears.shape.transform(gears.shape.rounded_bar)
--    : rotate(math.pi/2)
--       : translate(10, 10)
--
-- @param shape A shape function
-- @return A transformation handle, also act as a shape function
function module.transform(shape)
    local matrix = nil

    -- Apply the transformation matrix and apply the shape, then restore
    local function apply(self, cr, width, height, ...)
        cr:save()
        cr:set_matrix(self.matrix)
        shape(cr, width, height, ...)
        cr:restore()
    end

    local transformation = setmetatable({matrix=matrix}, {
        __call = apply,
        __index = function(_, key)
            return transformations[key]
        end
    })

    return transformation
end

return module
