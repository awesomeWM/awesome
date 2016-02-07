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
local g_matrix = require( "gears.matrix" )

local module = {}

--- Add a rounded rectangle to the current path.
-- Note: If the radius is bigger than either half side, it will be reduced.
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

--- A rounded rectangle with a triangle at the top
-- @param cr A cairo context
-- @tparam number width The shape with
-- @tparam number height The shape height
-- @tparam[opt=5] number corner_radius The corner radius
-- @tparam[opt=10] number arrow_size The width and height of the arrow
-- @tparam[opt=width/2 - arrow_size/2] number arrow_position The position of the arrow
function module.infobubble(cr, width, height, corner_radius, arrow_size, arrow_position)
    corner_radius  = corner_radius  or 5
    arrow_size     = arrow_size     or 10
    arrow_position = arrow_position or width/2 - arrow_size/2

    cr:move_to(0 ,corner_radius)

    -- Top left corner
    cr:arc(corner_radius, corner_radius+arrow_size, (corner_radius), math.pi, 3*(math.pi/2))

    -- The arrow triangle (still at the top)
    cr:line_to(arrow_position                , arrow_size )
    cr:line_to(arrow_position + arrow_size   , 0          )
    cr:line_to(arrow_position + 2*arrow_size , arrow_size )

    -- Complete the rounded rounded rectangle
    cr:arc(width-corner_radius, corner_radius+arrow_size  , (corner_radius) , 3*(math.pi/2) , math.pi*2 )
    cr:arc(width-corner_radius, height-(corner_radius)    , (corner_radius) , math.pi*2     , math.pi/2 )
    cr:arc(corner_radius      , height-(corner_radius)    , (corner_radius) , math.pi/2     , math.pi   )

    -- Close path
    cr:close_path()
end

--- A rectangle terminated by an arrow
-- @param cr A cairo context
-- @tparam number width The shape with
-- @tparam number height The shape height
function module.rectangular_tag(cr, width, height)
    cr:move_to(0        , height/2)
    cr:line_to(height/2 , 0       )
    cr:line_to(width    , 0       )
    cr:line_to(width    , height  )
    cr:line_to(height/2 , height  )

    cr:close_path()
end

--- A simple arrow shape
-- @param cr A cairo context
-- @tparam number width The shape with
-- @tparam number height The shape height
-- @tparam[opt=head_width] number head_width The width of the head (/\) of the arrow
-- @tparam[opt=width /2] number shaft_width The width of the shaft of the arrow
-- @tparam[opt=height/2] number shaft_length The head_length of the shaft (the rest is the head)
function module.arrow(cr, width, height, head_width, shaft_width, shaft_length)
    shaft_length = shaft_length or height / 2
    shaft_width  = shaft_width  or width  / 2
    head_width   = head_width   or width
    local head_length  = height - shaft_length

    cr:move_to    ( width/2                     , 0            )
    cr:rel_line_to( head_width/2                , head_length  )
    cr:rel_line_to( -(head_width-shaft_width)/2 , 0            )
    cr:rel_line_to( 0                           , shaft_length )
    cr:rel_line_to( -shaft_width                , 0            )
    cr:rel_line_to( 0           , -shaft_length                )
    cr:rel_line_to( -(head_width-shaft_width)/2 , 0            )

    cr:close_path()
end

--- A squeezed hexagon filling the rectangle
-- @param cr A cairo context
-- @tparam number width The shape with
-- @tparam number height The shape height
function module.hexagon(cr, width, height)
    cr:move_to(height/2,0)
    cr:line_to(width-height/2,0)
    cr:line_to(width,height/2)
    cr:line_to(width-height/2,height)
    cr:line_to(height/2,height)
    cr:line_to(0,height/2)
    cr:line_to(height/2,0)
    cr:close_path()
end

--- Double arrow popularized by the vim-powerline module
-- @param cr A cairo context
-- @tparam number width The shape with
-- @tparam number height The shape height
-- @tparam[opt=height/2] number arrow_depth The width of the arrow part of the shape
function module.powerline(cr, width, height, arrow_depth)
    arrow_depth = arrow_depth or height/2
    cr:move_to(0                   , 0        )
    cr:line_to(width - arrow_depth , 0        )
    cr:line_to(width               , height/2 )
    cr:line_to(width - arrow_depth , height   )
    cr:line_to(0                   , height   )
    cr:line_to(arrow_depth         , height/2 )

    cr:close_path()
end

--- An isosceles triangle
-- @param cr A cairo context
-- @tparam number width The shape with
-- @tparam number height The shape height
function module.isosceles_triangle(cr, width, height)
    cr:move_to( width/2, 0      )
    cr:line_to( width  , height )
    cr:line_to( 0      , height )
    cr:close_path()
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

    -- Apply the transformation matrix and apply the shape, then restore
    local function apply(self, cr, width, height, ...)
        cr:save()
        cr:transform(self:to_cairo_matrix())
        shape(cr, width, height, ...)
        cr:restore()
    end

    local matrix = g_matrix.identity:copy()
    rawset(matrix, "_call", apply)

    return matrix
end

return module
