---------------------------------------------------------------------------
-- A flexible separator widget.
--
-- By default, this widget display a simple line, but can be extended by themes
-- (or directly) to display much more complex visuals.
--
-- This widget is mainly intended to be used alongside the `spacing_widget`
-- property supported by various layouts such as:
--
-- * `wibox.layout.fixed`
-- * `wibox.layout.flex`
-- * `wibox.layout.ratio`
--
-- When used with these layouts, it is also possible to provide custom clipping
-- functions. This is useful when the layout has overlapping widgets (negative
-- spacing).
--
--@DOC_wibox_widget_defaults_separator_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2014, 2017 Emmanuel Lepage Vallee
-- @classmod wibox.widget.separator
---------------------------------------------------------------------------
local beautiful = require( "beautiful"         )
local base      = require( "wibox.widget.base" )
local color     = require( "gears.color"       )
local gtable    = require( "gears.table"       )

local separator = {}

--- The separator's orientation.
--
-- Valid values are:
--
-- * *vertical*: From top to bottom (default)
-- * *horizontal*: From left to right
--
--@DOC_wibox_widget_separator_orientation_EXAMPLE@
--
-- @property orientation
-- @param string

--- The separator's thickness.
--
-- This is used by the default line separator, but ignored when a shape is used.
--
-- @property thickness
-- @param number

--- The separator's reflection.
--
-- When a shape or a custom drawing function is used, the default shape assumes
-- this to go from left to right.
-- When this property is set to anything else, the shape will be rotated on one
-- axis or another.
--
-- This is the same as `wibox.container.mirror`, but need to exist on the widget
-- level because it affects other properties such as the layout clipping.
--
-- @property reflection
-- @tparam table reflection
-- @tparam[opt=false] boolean reflection.horizontal
-- @tparam[opt=false] boolean reflection.vertical

--- The separator's shape.
--
--@DOC_wibox_widget_separator_shape_EXAMPLE@
--
-- @property shape
-- @tparam function shape A valid shape function
-- @see gears.shape

--- The relative percentage covered by the bar.
-- @property span_ratio
-- @tparam[opt=1] number A number between 0 and 1.

--- The separator's color.
-- @property color
-- @param string
-- @see gears.color

--- The separator's border color.
--
--@DOC_wibox_widget_separator_border_color_EXAMPLE@
--
-- @property border_color
-- @param string
-- @see gears.color

--- The separator's border width.
-- @property border_width
-- @param number

local function draw_shape(self, _, cr, width, height, shape)
    local bw = self._private.border_width or 0

    cr:translate(bw/2, bw/2)

    shape(cr, width-bw, height-bw)

    if bw == 0 then
        cr:fill()
    elseif self._private.border_color then
        cr:fill_preserve()
        cr:set_source(color(self._private.border_color))
        cr:set_line_width(bw)
        cr:stroke()
    end
end

local function draw_line(self, _, cr, width, height)
    local thickness = self._private.thickness or beautiful.separator_thickness or 1

    local orientation = self._private.orientation or (
        width > height and "horizontal" or "vertical"
    )

    local span_ratio = self._private.span_ratio or 1

    if orientation == "horizontal" then
        local w = width*span_ratio
        cr:rectangle((width-w)/2, height/2 - thickness/2, w, thickness)
    else
        local h = height*span_ratio
        cr:rectangle(width/2 - thickness/2, (height-h)/2, thickness, h)
    end

    cr:fill()
end

local function draw(self, _, cr, width, height)
    -- In case there is a specialized.
    local draw_custom = self._private.draw or beautiful.separator_draw
    if draw_custom then
        return draw_custom(self, _, cr, width, height)
    end

    local col = self._private.color or beautiful.separator_color

    if col then
        cr:set_source(color(col))
    end

    local s = self._private.shape or beautiful.separator_shape

    if s then
        draw_shape(self, _, cr, width, height, s)
    else
        draw_line(self, _, cr, width, height)
    end
end

local function fit(_, _, width, height)
    return width, height
end

for _, prop in ipairs {"orientation", "color", "thickness", "span_ratio",
                       "border_width", "border_color", "shape" } do
    separator["set_"..prop] = function(self, value)
        self._private[prop] = value
        self:emit_signal("property::"..prop)
        if prop == "data_list" then
            self:emit_signal("property::data")
        end
        self:emit_signal("widget::redraw_needed")
    end
    separator["get_"..prop] = function(self)
        return self._private[prop] or beautiful["piechart_"..prop]
    end
end

local function new(args)
    local ret = base.make_widget(nil, nil, {
        enable_properties = true,
    })
    gtable.crush(ret, separator, true)
    gtable.crush(ret, args or {})
    rawset(ret, "fit" , fit )
    rawset(ret, "draw", draw)
    return ret
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(separator, { __call = function(_, ...) return new(...) end })
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
