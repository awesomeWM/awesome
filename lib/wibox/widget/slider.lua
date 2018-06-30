---------------------------------------------------------------------------
-- An interactive mouse based slider widget.
--
--@DOC_wibox_widget_defaults_slider_EXAMPLE@
--
-- @author Grigory Mishchenko &lt;grishkokot@gmail.com&gt;
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2015 Grigory Mishchenko, 2016 Emmanuel Lepage Vallee
-- @classmod wibox.widget.slider
---------------------------------------------------------------------------

local setmetatable = setmetatable
local type = type
local color = require("gears.color")
local gtable = require("gears.table")
local beautiful = require("beautiful")
local base = require("wibox.widget.base")
local shape = require("gears.shape")
local capi = {
    mouse        = mouse,
    mousegrabber = mousegrabber,
    root         = root,
}

local slider = {mt={}}

--- The slider handle shape.
--
--@DOC_wibox_widget_slider_handle_shape_EXAMPLE@
--
-- @property handle_shape
-- @tparam[opt=gears shape rectangle] gears.shape shape
-- @see gears.shape

--- The slider handle color.
--
--@DOC_wibox_widget_slider_handle_color_EXAMPLE@
--
-- @property handle_color
-- @param color

--- The slider handle margins.
--
--@DOC_wibox_widget_slider_handle_margins_EXAMPLE@
--
-- @property handle_margins
-- @tparam[opt={}] table margins
-- @tparam[opt=0] number margins.left
-- @tparam[opt=0] number margins.right
-- @tparam[opt=0] number margins.top
-- @tparam[opt=0] number margins.bottom

--- The slider handle width.
--
--@DOC_wibox_widget_slider_handle_width_EXAMPLE@
--
-- @property handle_width
-- @param number

--- The handle border_color.
--
--@DOC_wibox_widget_slider_handle_border_EXAMPLE@
--
-- @property handle_border_color
-- @param color

--- The handle border width.
-- @property handle_border_width
-- @param[opt=0]  number

--- The bar (background) shape.
--
--@DOC_wibox_widget_slider_bar_shape_EXAMPLE@
--
-- @property bar_shape
-- @tparam[opt=gears shape rectangle] gears.shape shape
-- @see gears.shape

--- The bar (background) height.
--
--@DOC_wibox_widget_slider_bar_height_EXAMPLE@
--
-- @property bar_height
-- @param number

--- The bar (background) color.
--
--@DOC_wibox_widget_slider_bar_color_EXAMPLE@
--
-- @property bar_color
-- @param color

--- The bar (background) margins.
--
--@DOC_wibox_widget_slider_bar_margins_EXAMPLE@
--
-- @property bar_margins
-- @tparam[opt={}] table margins
-- @tparam[opt=0] number margins.left
-- @tparam[opt=0] number margins.right
-- @tparam[opt=0] number margins.top
-- @tparam[opt=0] number margins.bottom

--- The bar (background) border width.
-- @property bar_border_width
-- @param[opt=0] numbergb

--- The bar (background) border_color.
--
--@DOC_wibox_widget_slider_bar_border_EXAMPLE@
--
-- @property bar_border_color
-- @param color

--- The slider value.
--
-- **Signal:** *property::value* notify the value is changed.
--
--@DOC_wibox_widget_slider_value_EXAMPLE@
--
-- @property value
-- @param[opt=0] number

--- The slider minimum value.
-- @property minimum
-- @param[opt=0] number

--- The slider maximum value.
-- @property maximum
-- @param[opt=100] number

--- The bar (background) border width.
-- @beautiful beautiful.slider_bar_border_width
-- @param number

--- The bar (background) border color.
-- @beautiful beautiful.slider_bar_border_color
-- @param color

--- The handle border_color.
-- @beautiful beautiful.slider_handle_border_color
-- @param color

--- The handle border width.
-- @beautiful beautiful.slider_handle_border_width
-- @param number

--- The handle .
-- @beautiful beautiful.slider_handle_width
-- @param number

-- @beautiful beautiful.slider_handle_color
-- @param color

--- The handle shape.
-- @beautiful beautiful.slider_handle_shape
-- @tparam[opt=gears shape rectangle] gears.shape shape
-- @see gears.shape

--- The bar (background) shape.
-- @beautiful beautiful.slider_bar_shape
-- @tparam[opt=gears shape rectangle] gears.shape shape
-- @see gears.shape

--- The bar (background) height.
-- @beautiful beautiful.slider_bar_height
-- @param number

--- The bar (background) margins.
-- @beautiful beautiful.slider_bar_margins
-- @tparam[opt={}] table margins
-- @tparam[opt=0] number margins.left
-- @tparam[opt=0] number margins.right
-- @tparam[opt=0] number margins.top
-- @tparam[opt=0] number margins.bottom

--- The slider handle margins.
-- @beautiful beautiful.slider_handle_margins
-- @tparam[opt={}] table margins
-- @tparam[opt=0] number margins.left
-- @tparam[opt=0] number margins.right
-- @tparam[opt=0] number margins.top
-- @tparam[opt=0] number margins.bottom

--- The bar (background) color.
-- @beautiful beautiful.slider_bar_color
-- @param color

local properties = {
    -- Handle
    handle_shape         = shape.rectangle,
    handle_color         = false,
    handle_margins       = {},
    handle_width         = false,
    handle_border_width  = 0,
    handle_border_color  = false,

    -- Bar
    bar_shape            = shape.rectangle,
    bar_height           = false,
    bar_color            = false,
    bar_margins          = {},
    bar_border_width     = 0,
    bar_border_color     = false,

    -- Content
    value                = 0,
    minimum              = 0,
    maximum              = 100,
}

-- Create the accessors
for prop in pairs(properties) do
    slider["set_"..prop] = function(self, value)
        local changed = self._private[prop] ~= value
        self._private[prop] = value

        if changed then
            self:emit_signal("property::"..prop)
            self:emit_signal("widget::redraw_needed")
        end
    end

    slider["get_"..prop] = function(self)
        -- Ignoring the false's is on purpose
        return self._private[prop] == nil
            and properties[prop]
            or self._private[prop]
    end
end

-- Add some validation to set_value
function slider:set_value(value)
    value = math.min(value, self:get_maximum())
    value = math.max(value, self:get_minimum())
    local changed = self._private.value ~= value

    self._private.value = value

    if changed then
        self:emit_signal( "property::value"       )
        self:emit_signal( "widget::redraw_needed" )
    end
end

local function get_extremums(self)
    local min = self._private.minimum or properties.minimum
    local max = self._private.maximum or properties.maximum
    local interval = max - min

    return min, max, interval
end

function slider:draw(_, cr, width, height)
    local bar_height = self._private.bar_height

    -- If there is no background, then skip this
    local bar_color = self._private.bar_color
        or beautiful.slider_bar_color

    if bar_color then
        cr:set_source(color(bar_color))
    end

    local margins = self._private.bar_margins
        or beautiful.slider_bar_margins

    local x_offset, right_margin, y_offset = 0, 0

    if margins then
        if type(margins) == "number" then
            bar_height = bar_height or (height - 2*margins)
            x_offset, y_offset = margins, margins
            right_margin = margins
        else
            bar_height = bar_height or (
                height - (margins.top or 0) - (margins.bottom or 0)
            )
            x_offset, y_offset = margins.left or 0, margins.top or 0
            right_margin = margins.right or 0
        end
    else
        bar_height = bar_height or beautiful.slider_bar_height or height
        y_offset   = (height - bar_height)/2
    end


    cr:translate(x_offset, y_offset)

    local bar_shape = self._private.bar_shape
        or beautiful.slider_bar_shape
        or properties.bar_shape

    local bar_border_width = self._private.bar_border_width
        or beautiful.slider_bar_border_width
        or properties.bar_border_width

    bar_shape(cr, width - x_offset - right_margin, bar_height or height)

    if bar_color then
        if bar_border_width == 0 then
            cr:fill()
        else
            cr:fill_preserve()
        end
    end

    -- Draw the bar border
    if bar_border_width > 0 then
        local bar_border_color = self._private.bar_border_color
            or beautiful.slider_bar_border_color
            or properties.bar_border_color

        cr:set_line_width(bar_border_width)

        if bar_border_color then
            cr:save()
            cr:set_source(color(bar_border_color))
            cr:stroke()
            cr:restore()
        else
            cr:stroke()
        end
    end

    cr:translate(-x_offset, -y_offset)

    -- Paint the handle
    local handle_color = self._private.handle_color
        or beautiful.slider_handle_color

    -- It is ok if there is no color, it will be inherited
    if handle_color then
        cr:set_source(color(handle_color))
    end

    local handle_height, handle_width = height, self._private.handle_width
        or beautiful.slider_handle_width
        or height/2

    local handle_shape = self._private.handle_shape
        or beautiful.slider_handle_shape
        or properties.handle_shape

    -- Lets get the margins for the handle
    margins = self._private.handle_margins
        or beautiful.slider_handle_margins

    x_offset, y_offset = 0, 0

    if margins then
        if type(margins) == "number" then
            x_offset, y_offset = margins, margins
            handle_width  = handle_width  - 2*margins
            handle_height = handle_height - 2*margins
        else
            x_offset, y_offset = margins.left or 0, margins.top or 0
            handle_width  = handle_width  -
                (margins.left or 0) - (margins.right  or 0)
            handle_height = handle_height -
                (margins.top  or 0) - (margins.bottom or 0)
        end
    end

    local value = self._private.value or self._private.min or 0

    -- Get the widget size back to it's non-transfored value
    local min, _, interval = get_extremums(self)
    local rel_value = ((value-min)/interval) * (width-handle_width)

    cr:translate(x_offset + rel_value, y_offset)

    local handle_border_width = self._private.handle_border_width
        or beautiful.slider_handle_border_width
        or properties.handle_border_width or 0

    handle_shape(cr, handle_width, handle_height)

    if handle_border_width > 0 then
        cr:fill_preserve()
    else
        cr:fill()
    end

    -- Draw the handle border
    if handle_border_width > 0 then
        local handle_border_color = self._private.handle_border_color
            or beautiful.slider_handle_border_color
            or properties.handle_border_color

        if handle_border_color then
            cr:set_source(color(handle_border_color))
        end

        cr:set_line_width(handle_border_width)
        cr:stroke()
    end
end

function slider:fit(_, width, height)
    -- Use all the space, this should be used with a constraint widget
    return width, height
end

-- Move the handle to the correct location
local function move_handle(self, width, x, _)
    local _, _, interval = get_extremums(self)
    self:set_value(math.floor((x*interval)/width))
end

local function mouse_press(self, x, y, button_id, _, geo)
    if button_id ~= 1 then return end

    local matrix_from_device = geo.hierarchy:get_matrix_from_device()

    -- Sigh. geo.width/geo.height is in device space. We need it in our own
    -- coordinate system
    local width = geo.widget_width

    move_handle(self, width, x, y)

    -- Calculate a matrix transforming from screen coordinates into widget coordinates
    local wgeo = geo.drawable.drawable:geometry()
    local matrix = matrix_from_device:translate(-wgeo.x, -wgeo.y)

    capi.mousegrabber.run(function(mouse)
        if not mouse.buttons[1] then
            return false
        end

        -- Calculate the point relative to the widget
        move_handle(self, width, matrix:transform_point(mouse.x, mouse.y))

        return true
    end,"fleur")
end

--- Create a slider widget.
-- @tparam[opt={}] table args
-- @function wibox.widget.slider
local function new(args)
    local ret = base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(ret._private, args or {})

    gtable.crush(ret, slider, true)

    ret:connect_signal("button::press", mouse_press)

    return ret
end

function slider.mt:__call(_, ...)
    return new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(slider, slider.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
