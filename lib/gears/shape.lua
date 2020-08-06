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
-- to actually draw the content, use `cr:fill()`, `cr:mask()`, `cr:clip()` or
-- `cr:stroke()`
--
-- In many case, it is necessary to apply the shape using a transformation
-- such as a rotation. The preferred way to do this is to wrap the function
-- in another function calling `cr:rotate()` (or any other transformation
-- matrix).
--
-- To specialize a shape where the API doesn't allows extra arguments to be
-- passed, it is possible to wrap the shape function like:
--
--    local new_shape = function(cr, width, height)
--        gears.shape.rounded_rect(cr, width, height, 2)
--    end
--
-- Many elements can be shaped. This include:
--
-- * `client`s (see `gears.surface.apply_shape_bounding`)
-- * `wibox`es (see `wibox.shape`)
-- * All widgets (see `wibox.container.background`)
-- * The progressbar (see `wibox.widget.progressbar.bar_shape`)
-- * The graph (see `wibox.widget.graph.step_shape`)
-- * The checkboxes (see `wibox.widget.checkbox.check_shape`)
-- * Images (see `wibox.widget.imagebox.clip_shape`)
-- * The taglist tags (see `awful.widget.taglist`)
-- * The tasklist clients (see `awful.widget.tasklist`)
-- * The tooltips (see `awful.tooltip`)
--
-- @author Emmanuel Lepage Vallee
-- @copyright 2011-2016 Emmanuel Lepage Vallee
-- @themelib gears.shape
---------------------------------------------------------------------------
local g_matrix = require( "gears.matrix" )
local g_math   = require( "gears.math" )
local unpack   = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local atan2    = math.atan2 or math.atan -- lua 5.3 compat
local min      = math.min
local max      = math.max
local cos      = math.cos
local sin      = math.sin
local abs      = math.abs
local pow      = math.pow -- luacheck: globals math.pow
if not pow then
  -- math.pow can be disabled in Lua 5.3 via LUA_COMPAT_MATHLIB
  pow = function(x, y)
        return x^y
    end
end

local module = {}

--- Add a squircle shape with only some of the corner are "circled" to the current path.
-- The squircle is not exactly as the definition.
-- It will expand to the shape's width and height, kinda like an ellipse
--
-- @DOC_gears_shape_partial_squircle_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam boolean tl If the top left corner is rounded
-- @tparam boolean tr If the top right corner is rounded
-- @tparam boolean br If the bottom right corner is rounded
-- @tparam boolean bl If the bottom left corner is rounded
-- @tparam number rate The "squareness" of the squircle, should be greater than 1
-- @tparam number delta The "smoothness" of the shape, delta must be greater than 0.01 and will be reset to 0.01 if not
-- @staticfct gears.shape.partial_squircle
function module.partial_squircle(cr, width, height, tl, tr, br, bl, rate, delta)
   -- rate ~ 2 can be used by icon
   -- this shape doesn't really fit clients
   -- but you can still use with rate ~ 32
   rate = rate or 2
   -- smaller the delta the smoother the shape
   -- but probably more laggy
   -- so we'll limit delta to the miminum of 0.01
   -- so people don't burn their computer
   delta = delta or 1 / max(width, height)
   delta = delta > 0.01 and delta or 0.01

   -- just ellipse things
   local a = width / 2
   local b = height / 2
   local phi = 0

   -- move to origin
   -- the shape goes counter clock wise
   -- start from (w h / 2)
   cr:save()
   cr:translate(a, b)
   cr:move_to(a, 0)

   -- draw the corner if that corner is rounded
   local curved_corner = function()
      local end_angle = phi + math.pi / 2
      while phi < end_angle do
         local cosphi = cos(phi)
         local sinphi = sin(phi)
         local x = a * pow(abs(cosphi), 1 / rate) * g_math.sign(cosphi)
         local y = b * pow(abs(sinphi), 1 / rate) * g_math.sign(sinphi)
         -- so weird, y axis is inverted
         cr:line_to(x, -y)
         phi = phi + delta
      end
   end

   -- draw with polar cord
   -- draw top right
   if tr then
      curved_corner()
   else
      cr:move_to(a,  0)
      cr:line_to(a, -b)
      cr:line_to(0, -b)
      phi = math.pi * 0.5
   end

   -- draw top left
   if tl then
      curved_corner()
   else
      cr:line_to(-a, -b)
      cr:line_to(-a,  0)
      phi = math.pi
   end

   if bl then
      curved_corner()
   else
      cr:line_to(-a, b)
      cr:line_to( 0, b)
      phi = math.pi * 1.5
   end

   -- bottom right
   if br then
      curved_corner()
   else
      cr:line_to(a, b)
      cr:line_to(a, 0)
      phi = math.pi * 2
   end

   -- it's time to stop
   cr:close_path()

   -- restore cairo context
   cr:restore()
end

--- Add a squircle shape to the current path.
-- This will behave the same as `partial_squircle`
--
-- @DOC_gears_shape_squircle_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam number rate The "squareness" of the squircle, should be greater than 1
-- @tparam number delta The "smoothness" of the shape, delta must be greater than 0.01 and will be reset to 0.01 if not
-- @staticfct gears.shape.squircle
function module.squircle(cr, width, height, rate, delta)
   module.partial_squircle(cr, width, height, true, true, true, true, rate, delta)
end

--- Add a star shape to the current path.
-- The star size will be the minimum of the given width and weight
--
-- @DOC_gears_shape_star_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The width constraint
-- @tparam number height The height constraint
-- @tparam number n Number of grams (default n = 5 -> pentagram)
-- @staticfct gears.shape.star
function module.star(cr, width, height, n)
    -- use the minimum as size
    local s = min(width, height) / 2

    -- draw pentagram by default
    n = n or 5
    local a = 2 * math.pi / n

    -- place the star at the center
    cr:save()
    cr:translate(width/2, height/2)
    cr:rotate(-math.pi/2)

    for i = 0,(n - 1) do
        cr:line_to(s   * cos((i      ) * a), s   * sin((i      ) * a))
        cr:line_to(s/2 * cos((i + 0.5) * a), s/2 * sin((i + 0.5) * a))
    end

    -- restore the context
    cr:restore()

    cr:close_path()
end

--- Add a rounded rectangle to the current path.
-- Note: If the radius is bigger than either half side, it will be reduced.
--
-- @DOC_gears_shape_rounded_rect_EXAMPLE@
--
-- @param cr A cairo content
-- @tparam number width The rectangle width
-- @tparam number height The rectangle height
-- @tparam number radius the corner radius
-- @staticfct gears.shape.rounded_rect
function module.rounded_rect(cr, width, height, radius)

    radius = radius or 10

    if width / 2 < radius then
        radius = width / 2
    end

    if height / 2 < radius then
        radius = height / 2
    end

    cr:move_to(0, radius)

    cr:arc( radius      , radius       , radius,    math.pi   , 3*(math.pi/2) )
    cr:arc( width-radius, radius       , radius, 3*(math.pi/2),    math.pi*2  )
    cr:arc( width-radius, height-radius, radius,    math.pi*2 ,    math.pi/2  )
    cr:arc( radius      , height-radius, radius,    math.pi/2 ,    math.pi    )

    cr:close_path()
end

--- Add a rectangle delimited by 2 180 degree arcs to the path.
--
-- @DOC_gears_shape_rounded_bar_EXAMPLE@
--
-- @param cr A cairo content
-- @param width The rectangle width
-- @param height The rectangle height.
-- @staticfct gears.shape.rounded_bar
function module.rounded_bar(cr, width, height)
    module.rounded_rect(cr, width, height, height / 2)
end

--- A rounded rect with only some of the corners rounded.
--
-- @DOC_gears_shape_partially_rounded_rect_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam boolean tl If the top left corner is rounded
-- @tparam boolean tr If the top right corner is rounded
-- @tparam boolean br If the bottom right corner is rounded
-- @tparam boolean bl If the bottom left corner is rounded
-- @tparam number rad The corner radius
-- @staticfct gears.shape.partially_rounded_rect
function module.partially_rounded_rect(cr, width, height, tl, tr, br, bl, rad)
    rad = rad or 10
    if width / 2 < rad then
        rad = width / 2
    end

    if height / 2 < rad then
        rad = height / 2
    end

    -- In case there is already some other path on the cairo context:
    -- Make sure the close_path() below goes to the right position.
    cr:new_sub_path()

    -- Top left
    if tl then
        cr:arc( rad, rad, rad, math.pi, 3*(math.pi/2))
    else
        cr:move_to(0,0)
    end

    -- Top right
    if tr then
        cr:arc( width-rad, rad, rad, 3*(math.pi/2), math.pi*2)
    else
        cr:line_to(width, 0)
    end

    -- Bottom right
    if br then
        cr:arc( width-rad, height-rad, rad, math.pi*2 , math.pi/2)
    else
        cr:line_to(width, height)
    end

    -- Bottom left
    if bl then
        cr:arc( rad, height-rad, rad, math.pi/2, math.pi)
    else
        cr:line_to(0, height)
    end

    cr:close_path()
end

--- A rounded rectangle with a triangle at the top.
--
-- @DOC_gears_shape_infobubble_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam[opt=5] number corner_radius The corner radius
-- @tparam[opt=10] number arrow_size The width and height of the arrow
-- @tparam[opt=width/2 - arrow_size/2] number arrow_position The position of the arrow
-- @staticfct gears.shape.infobubble
function module.infobubble(cr, width, height, corner_radius, arrow_size, arrow_position)
    arrow_size     = arrow_size     or 10
    corner_radius  = math.min((height-arrow_size)/2, corner_radius or 5)
    arrow_position = arrow_position or width/2 - arrow_size/2


    cr:move_to(0 ,corner_radius+arrow_size)

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

--- A rectangle terminated by an arrow.
--
-- @DOC_gears_shape_rectangular_tag_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam[opt=height/2] number arrow_length The length of the arrow part
-- @staticfct gears.shape.rectangular_tag
function module.rectangular_tag(cr, width, height, arrow_length)
    arrow_length = arrow_length or height/2
    if arrow_length > 0 then
        cr:move_to(0            , height/2 )
        cr:line_to(arrow_length , 0        )
        cr:line_to(width        , 0        )
        cr:line_to(width        , height   )
        cr:line_to(arrow_length , height   )
    else
        cr:move_to(0            , 0        )
        cr:line_to(-arrow_length, height/2 )
        cr:line_to(0            , height   )
        cr:line_to(width        , height   )
        cr:line_to(width        , 0        )
    end

    cr:close_path()
end

--- A simple arrow shape.
--
-- @DOC_gears_shape_arrow_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam[opt=head_width] number head_width The width of the head (/\) of the arrow
-- @tparam[opt=width /2] number shaft_width The width of the shaft of the arrow
-- @tparam[opt=height/2] number shaft_length The head_length of the shaft (the rest is the head)
-- @staticfct gears.shape.arrow
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

--- A squeezed hexagon filling the rectangle.
--
-- @DOC_gears_shape_hexagon_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @staticfct gears.shape.hexagon
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

--- Double arrow popularized by the vim-powerline module.
--
-- @DOC_gears_shape_powerline_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam[opt=height/2] number arrow_depth The width of the arrow part of the shape
-- @staticfct gears.shape.powerline
function module.powerline(cr, width, height, arrow_depth)
    arrow_depth = arrow_depth or height/2
    local offset = 0

    -- Avoid going out of the (potential) clip area
    if arrow_depth < 0 then
        width  =  width + 2*arrow_depth
        offset = -arrow_depth
    end

    cr:move_to(offset                       , 0        )
    cr:line_to(offset + width - arrow_depth , 0        )
    cr:line_to(offset + width               , height/2 )
    cr:line_to(offset + width - arrow_depth , height   )
    cr:line_to(offset                       , height   )
    cr:line_to(offset + arrow_depth         , height/2 )

    cr:close_path()
end

--- An isosceles triangle.
--
-- @DOC_gears_shape_isosceles_triangle_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @staticfct gears.shape.isosceles_triangle
function module.isosceles_triangle(cr, width, height)
    cr:move_to( width/2, 0      )
    cr:line_to( width  , height )
    cr:line_to( 0      , height )
    cr:close_path()
end

--- A cross (**+**) symbol.
--
-- @DOC_gears_shape_cross_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam[opt=width/3] number thickness The cross section thickness
-- @staticfct gears.shape.cross
function module.cross(cr, width, height, thickness)
    thickness = thickness or width/3
    local xpadding   = (width  - thickness) / 2
    local ypadding   = (height - thickness) / 2
    cr:move_to(xpadding, 0)
    cr:line_to(width - xpadding, 0)
    cr:line_to(width - xpadding, ypadding)
    cr:line_to(width           , ypadding)
    cr:line_to(width           , height-ypadding)
    cr:line_to(width - xpadding, height-ypadding)
    cr:line_to(width - xpadding, height         )
    cr:line_to(xpadding        , height         )
    cr:line_to(xpadding        , height-ypadding)
    cr:line_to(0               , height-ypadding)
    cr:line_to(0               , ypadding       )
    cr:line_to(xpadding        , ypadding       )
    cr:close_path()
end

--- A similar shape to the `rounded_rect`, but with sharp corners.
--
-- @DOC_gears_shape_octogon_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam number corner_radius
-- @staticfct gears.shape.octogon
function module.octogon(cr, width, height, corner_radius)
    corner_radius = corner_radius or math.min(10, math.min(width, height)/4)
    local offset = math.sqrt( (corner_radius*corner_radius) / 2 )

    cr:move_to(offset, 0)
    cr:line_to(width-offset, 0)
    cr:line_to(width, offset)
    cr:line_to(width, height-offset)
    cr:line_to(width-offset, height)
    cr:line_to(offset, height)
    cr:line_to(0, height-offset)
    cr:line_to(0, offset)
    cr:close_path()
end

--- A circle shape.
--
-- @DOC_gears_shape_circle_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam[opt=math.min(width  height) / 2)] number radius The radius
-- @staticfct gears.shape.circle
function module.circle(cr, width, height, radius)
    radius = radius or math.min(width, height) / 2
    cr:move_to(width/2+radius, height/2)
    cr:arc(width / 2, height / 2, radius, 0, 2*math.pi)
    cr:close_path()
end

--- A simple rectangle.
--
-- @DOC_gears_shape_rectangle_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @staticfct gears.shape.rectangle
function module.rectangle(cr, width, height)
    cr:rectangle(0, 0, width, height)
end

--- A diagonal parallelogram with the bottom left corner at x=0 and top right
-- at x=width.
--
-- @DOC_gears_shape_parallelogram_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam[opt=width/3] number base_width The parallelogram base width
-- @staticfct gears.shape.parallelogram
function module.parallelogram(cr, width, height, base_width)
    base_width = base_width or width/3
    cr:move_to(width-base_width, 0      )
    cr:line_to(width           , 0      )
    cr:line_to(base_width      , height )
    cr:line_to(0               , height )
    cr:close_path()
end

--- A losange.
--
-- @DOC_gears_shape_losange_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @staticfct gears.shape.losange
function module.losange(cr, width, height)
    cr:move_to(width/2 , 0        )
    cr:line_to(width   , height/2 )
    cr:line_to(width/2 , height   )
    cr:line_to(0       , height/2 )
    cr:close_path()
end

--- A pie.
--
-- The pie center is the center of the area.
--
-- @DOC_gears_shape_pie_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam[opt=0] number start_angle The start angle (in radian)
-- @tparam[opt=math.pi/2] number end_angle The end angle (in radian)
-- @tparam[opt=math.min(width height)/2] number radius The shape height
-- @staticfct gears.shape.pie
function module.pie(cr, width, height, start_angle, end_angle, radius)
    radius = radius or math.floor(math.min(width, height)/2)
    start_angle, end_angle = start_angle or 0, end_angle or math.pi/2

    -- In case there is already some other path on the cairo context:
    -- Make sure the close_path() below goes to the right position.
    cr:new_sub_path()

    -- If the shape is a circle, then avoid the lines
    if math.abs(start_angle + end_angle - 2*math.pi) <= 0.01  then
        cr:arc(width/2, height/2, radius, 0, 2*math.pi)
    else
        cr:move_to(width/2, height/2)
        cr:line_to(
            width/2 + math.cos(start_angle)*radius,
            height/2 + math.sin(start_angle)*radius
        )
        cr:arc(width/2, height/2, radius, start_angle, end_angle)
    end

    cr:close_path()
end

--- A rounded arc.
--
-- The pie center is the center of the area.
--
-- @DOC_gears_shape_arc_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number width The shape width
-- @tparam number height The shape height
-- @tparam[opt=math.min(width height)/2] number thickness The arc thickness
-- @tparam[opt=0] number start_angle The start angle (in radian)
-- @tparam[opt=math.pi/2] number end_angle The end angle (in radian)
-- @tparam[opt=false] boolean start_rounded if the arc start rounded
-- @tparam[opt=false] boolean end_rounded if the arc end rounded
-- @staticfct gears.shape.arc
function module.arc(cr, width, height, thickness, start_angle, end_angle, start_rounded, end_rounded)
    start_angle = start_angle or 0
    end_angle   = end_angle   or math.pi/2

    -- In case there is already some other path on the cairo context:
    -- Make sure the close_path() below goes to the right position.
    cr:new_sub_path()

    -- This shape is a partial circle
    local radius = math.min(width, height)/2

    thickness = thickness or radius/2

    local inner_radius = radius - thickness

    -- As the edge of the small arc need to touch the [start_p1, start_p2]
    -- line, a small subset of the arc circumference has to be substracted
    -- that's (less or more) equal to the thickness/2 (a little longer given
    -- it is an arc and not a line, but it wont show)
    local arc_percent = math.abs(end_angle-start_angle)/(2*math.pi)
    local arc_length  = ((radius-thickness/2)*2*math.pi)*arc_percent

    if start_rounded then
        arc_length = arc_length - thickness/2

        -- And back to angles
        start_angle = end_angle - (arc_length/(radius - thickness/2))
    end

    if end_rounded then
        arc_length = arc_length - thickness/2

        -- And back to angles. Also make sure to avoid underflowing when the
        -- rounded edge radius is greater than the angle delta.
        end_angle = start_angle + math.max(
            0, arc_length/(radius - thickness/2)
        )
    end

    -- The path is a curcular arc joining 4 points

    -- Outer first corner
    local start_p1 = {
        width /2 + math.cos(start_angle)*radius,
        height/2 + math.sin(start_angle)*radius
    }

    if start_rounded then

        -- Inner first corner
        local start_p2 = {
            width /2 + math.cos(start_angle)*inner_radius,
            height/2 + math.sin(start_angle)*inner_radius
        }

        local median_angle = atan2(
              start_p2[1] - start_p1[1],
            -(start_p2[2] - start_p1[2])
        )

        local arc_center = {
            (start_p1[1] + start_p2[1])/2,
            (start_p1[2] + start_p2[2])/2,
        }

        cr:arc(arc_center[1], arc_center[2], thickness/2,
            median_angle-math.pi/2, median_angle+math.pi/2
        )

    else
        cr:move_to(unpack(start_p1))
    end

    cr:arc(width/2, height/2, radius, start_angle, end_angle)

    if end_rounded then

        -- Outer second corner
        local end_p1 = {
            width /2 + math.cos(end_angle)*radius,
            height/2 + math.sin(end_angle)*radius
        }

        -- Inner first corner
        local end_p2 = {
            width /2 + math.cos(end_angle)*inner_radius,
            height/2 + math.sin(end_angle)*inner_radius
        }
        local median_angle = atan2(
              end_p2[1] - end_p1[1],
            -(end_p2[2] - end_p1[2])
        ) - math.pi

        local arc_center = {
            (end_p1[1] + end_p2[1])/2,
            (end_p1[2] + end_p2[2])/2,
        }

        cr:arc(arc_center[1], arc_center[2], thickness/2,
            median_angle-math.pi/2, median_angle+math.pi/2
        )

    end

    cr:arc_negative(width/2, height/2, inner_radius, end_angle, start_angle)

    cr:close_path()
end

--- A partial rounded bar. How much of the rounded bar is visible depends on
-- the given percentage value.
--
-- Note that this shape is not closed and thus filling it doesn't make much
-- sense.
--
-- @DOC_gears_shape_radial_progress_EXAMPLE@
--
-- @param cr A cairo context
-- @tparam number w The shape width
-- @tparam number h The shape height
-- @tparam[opt=1] number percent The progressbar percent
-- @tparam[opt=false] boolean hide_left Do not draw the left side of the shape
-- @tparam[opt=math.min(width height) / 2] number radius The arc radius
-- @staticfct gears.shape.radial_progress
function module.radial_progress(cr, w, h, percent, hide_left, radius)
    percent = percent or 1
    radius  = radius  or (math.min(w, h) / 2)

    if radius > math.min(w, h) / 2 then
        radius = math.min(w, h) / 2
    end

    local no_hline = radius == (2*w)
    local no_vline = radius == (2*h)

    local total_length  = 2*(w + h) - 8*radius + 2*math.pi*radius
    local hbar_percent  = (w - 2*radius)/total_length
    local vbar_percent  = (h - 2*radius)/total_length
    local arc_percent   = (math.pi*radius/2)/total_length

    -- Bottom line
    if not no_hline then
        if percent > hbar_percent then
            cr:move_to(radius, h)
            cr:line_to(radius + (w - 2*radius), h)
            cr:stroke()
        elseif percent < hbar_percent then
            cr:move_to(radius, h)
            cr:line_to(radius + total_length*percent, h)
            cr:stroke()
        end
    end

    -- Bottom right arc
    if percent >= hbar_percent + arc_percent then
        cr:arc(w - radius, h - radius, radius, 0, math.pi/2)
        cr:stroke()
    elseif percent > hbar_percent and percent < hbar_percent + arc_percent then
        local relPercent = percent - hbar_percent
        cr:arc_negative(w - radius , h - radius, radius,
                        math.pi/2,
                        math.pi/2 - total_length*relPercent/radius
        )
        cr:stroke()
    end

    -- Right line
    if not no_vline then
        if percent >= hbar_percent + arc_percent + vbar_percent then
            cr:move_to(w, h - radius)
            cr:line_to(w, radius)
            cr:stroke()
        elseif percent > hbar_percent + arc_percent
        and percent < hbar_percent + arc_percent + vbar_percent then
            local relPercent = percent - hbar_percent - arc_percent
            cr:move_to(w, h - radius)
            cr:line_to(w, h - radius - total_length*relPercent)
            cr:stroke()
        end
    end

    -- Top right arc
    if percent >= hbar_percent + 2*arc_percent + vbar_percent then
        cr:arc_negative(w - radius, radius, radius, 0, 3*math.pi/2)
        cr:stroke()
    elseif percent > hbar_percent + arc_percent + vbar_percent
    and percent < hbar_percent + 2*arc_percent + vbar_percent then
        local relPercent = percent - hbar_percent - arc_percent - vbar_percent
        cr:arc_negative(w - radius, radius, radius,
                        0,
                        - total_length*relPercent/radius
        )
        cr:stroke()
    end

    -- Top line
    if not no_hline then
        if percent >= 2*hbar_percent + 2*arc_percent + vbar_percent then
            cr:move_to(w - radius, 0)
            cr:line_to(radius, 0)
            cr:stroke()
        elseif percent > hbar_percent + 2*arc_percent + vbar_percent
        and percent < 2*hbar_percent + 2*arc_percent + vbar_percent then
            local relPercent = percent - hbar_percent - 2*arc_percent - vbar_percent
            cr:move_to(w - radius, 0)
            cr:line_to(w - radius - total_length*relPercent, 0)
            cr:stroke()
        end
    end

    if not hide_left then
        -- Top left arc
        if percent >= 2*hbar_percent + 3*arc_percent + vbar_percent then
            cr:arc_negative(radius, radius, radius, -math.pi/2, math.pi)
            cr:stroke()
        elseif percent > 2*hbar_percent + 2*arc_percent + vbar_percent
        and percent < 2*hbar_percent + 3*arc_percent + vbar_percent then
            local relPercent = percent - 2*hbar_percent - 2*arc_percent - vbar_percent
            cr:arc_negative(radius, radius, radius,
                            -math.pi/2,
                            -math.pi/2 - total_length*relPercent/radius
            )
            cr:stroke()
        end

        -- Left line
        if not no_vline then
            if percent >= 2*hbar_percent + 3*arc_percent + 2*vbar_percent then
                cr:move_to(0, radius)
                cr:line_to(0, h - radius)
                cr:stroke()
            elseif percent > 2*hbar_percent + 3*arc_percent + vbar_percent
            and percent < 2*hbar_percent + 3*arc_percent + 2*vbar_percent then
                local relPercent = percent - 2*hbar_percent - 3*arc_percent - vbar_percent
                cr:move_to(0, radius)
                cr:line_to(0, radius + total_length*relPercent)
                cr:stroke()
            end
        end

        -- Bottom left arc
        if percent >= 1 - 1.5e-2 then
            cr:arc_negative(radius, h - radius, radius, math.pi, math.pi/2)
            cr:stroke()
        elseif percent > 2*hbar_percent + 3*arc_percent + 2*vbar_percent then
            local relPercent = percent - 2*hbar_percent - 3*arc_percent - 2*vbar_percent
            cr:arc_negative(radius, h - radius, radius,
                            math.pi,
                            math.pi - total_length*relPercent/radius
            )
            cr:stroke()
        end
    end
end

--- Adjust the shape using a transformation object
--
-- Apply various transformations to the shape
--
-- @usage gears.shape.transform(gears.shape.rounded_bar)
--    : rotate(math.pi/2)
--       : translate(10, 10)
--
-- @param shape A shape function
-- @return A transformation handle, also act as a shape function
-- @staticfct gears.shape.transform
function module.transform(shape)

    -- Apply the transformation matrix and apply the shape, then restore
    local function apply(self, cr, width, height, ...)
        cr:save()
        cr:transform(self.matrix:to_cairo_matrix())
        shape(cr, width, height, ...)
        cr:restore()
    end
    -- Redirect function calls like :rotate() to the underlying matrix
    local function index(_, key)
        return function(self, ...)
            self.matrix = self.matrix[key](self.matrix, ...)
            return self
        end
    end

    local result = setmetatable({
        matrix = g_matrix.identity
    }, {
        __call = apply,
        __index = index
    })

    return result
end

return module

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
