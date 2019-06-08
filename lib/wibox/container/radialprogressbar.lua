---------------------------------------------------------------------------
--
-- A circular progressbar wrapper.
--
-- If no child `widget` is set, then the radialprogressbar will take all the
-- available size. Use a `wibox.container.constraint` to prevent this.
--
--@DOC_wibox_container_defaults_radialprogressbar_EXAMPLE@
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2013 Emmanuel Lepage Vallee
-- @containermod wibox.container.radialprogressbar
---------------------------------------------------------------------------

local setmetatable = setmetatable
local base      = require("wibox.widget.base")
local shape     = require("gears.shape"      )
local gtable    = require( "gears.table"     )
local color     = require( "gears.color"     )
local beautiful = require("beautiful"        )

local default_outline_width  = 2

local radialprogressbar = { mt = {} }

--- The progressbar border background color.
-- @beautiful beautiful.radialprogressbar_border_color
-- @param color

--- The progressbar foreground color.
-- @beautiful beautiful.radialprogressbar_color
-- @param color

--- The progressbar border width.
-- @beautiful beautiful.radialprogressbar_border_width
-- @param number

--- The padding between the outline and the progressbar.
-- @beautiful beautiful.radialprogressbar_paddings
-- @tparam[opt=0] table|number paddings A number or a table
-- @tparam[opt=0] number paddings.top
-- @tparam[opt=0] number paddings.bottom
-- @tparam[opt=0] number paddings.left
-- @tparam[opt=0] number paddings.right

local function outline_workarea(self, width, height)
    local border_width = self._private.border_width or
        beautiful.radialprogressbar_border_width or default_outline_width

    local x, y = 0, 0

    -- Make sure the border fit in the clip area
    local offset = border_width/2
    x, y = x + offset, y+offset
    width, height = width-2*offset, height-2*offset

    return {x=x, y=y, width=width, height=height}, offset
end

-- The child widget area
local function content_workarea(self, width, height)
    local padding = self._private.paddings or {}
    local wa = outline_workarea(self, width, height)

    wa.x      = wa.x + (padding.left or 0)
    wa.y      = wa.y + (padding.top  or 0)
    wa.width  = wa.width  - (padding.left or 0) - (padding.right  or 0)
    wa.height = wa.height - (padding.top  or 0) - (padding.bottom or 0)

    return wa
end

-- Draw the radial outline and progress
function radialprogressbar:after_draw_children(_, cr, width, height)
    cr:restore()

    local border_width = self._private.border_width or
        beautiful.radialprogressbar_border_width or default_outline_width

    local wa = outline_workarea(self, width, height)
    cr:translate(wa.x, wa.y)

    -- Draw the outline
    shape.rounded_bar(cr, wa.width, wa.height)
    cr:set_source(color(self:get_border_color() or "#0000ff"))
    cr:set_line_width(border_width)
    cr:stroke()

    -- Draw the progress
    cr:set_source(color(self:get_color() or "#ff00ff"))
    shape.radial_progress(cr,  wa.width, wa.height, self._percent or 0)
    cr:set_line_width(border_width)
    cr:stroke()

end

-- Set the clip
function radialprogressbar:before_draw_children(_, cr, width, height)
    cr:save()
    local wa = content_workarea(self, width, height)
    cr:translate(wa.x, wa.y)
    shape.rounded_bar(cr, wa.width, wa.height)
    cr:clip()
    cr:translate(-wa.x, -wa.y)
end

-- Layout this layout
function radialprogressbar:layout(_, width, height)
    if self._private.widget then
        local wa = content_workarea(self, width, height)

        return { base.place_widget_at(
            self._private.widget, wa.x, wa.y, wa.width, wa.height
        ) }
    end
end

-- Fit this layout into the given area
function radialprogressbar:fit(context, width, height)
    if self._private.widget then
        local wa = content_workarea(self, width, height)
        local w, h = base.fit_widget(self, context, self._private.widget, wa.width, wa.height)
        return wa.x + w, wa.y + h
    end

    return width, height
end

--- The widget to wrap in a radial proggressbar.
-- @property widget
-- @tparam widget widget The widget

function radialprogressbar:set_widget(widget)
    if widget then
        base.check_widget(widget)
    end
    self._private.widget = widget
    self:emit_signal("widget::layout_changed")
end

function radialprogressbar:get_children()
    return {self._private.widget}
end

function radialprogressbar:set_children(children)
    self._private.widget = children and children[1]
    self:emit_signal("widget::layout_changed")
end

--- Reset this container.
-- @method reset
function radialprogressbar:reset()
    self:set_widget(nil)
end

for _,v in ipairs {"left", "right", "top", "bottom"} do
    radialprogressbar["set_"..v.."_padding"] = function(self, val)
        self._private.paddings = self._private.paddings or {}
        self._private.paddings[v] = val
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("widget::layout_changed")
    end
end

--- The padding between the outline and the progressbar.
--@DOC_wibox_container_radialprogressbar_padding_EXAMPLE@
-- @property paddings
-- @tparam[opt=0] table|number paddings A number or a table
-- @tparam[opt=0] number paddings.top
-- @tparam[opt=0] number paddings.bottom
-- @tparam[opt=0] number paddings.left
-- @tparam[opt=0] number paddings.right

--- The progressbar value.
--@DOC_wibox_container_radialprogressbar_value_EXAMPLE@
-- @property value
-- @tparam number value Between min_value and max_value

function radialprogressbar:set_value(val)
    if not val then self._percent = 0; return end

    if val > self._private.max_value then
        self:set_max_value(val)
    elseif val < self._private.min_value then
        self:set_min_value(val)
    end

    local delta = self._private.max_value - self._private.min_value

    self._percent = val/delta
    self:emit_signal("widget::redraw_needed")
end

--- The border background color.
--@DOC_wibox_container_radialprogressbar_border_color_EXAMPLE@
-- @property border_color
-- @param color

--- The border foreground color.
--@DOC_wibox_container_radialprogressbar_color_EXAMPLE@
-- @property color
-- @param color

--- The border width.
--@DOC_wibox_container_radialprogressbar_border_width_EXAMPLE@
-- @property border_width
-- @tparam[opt=3] number border_width

--- The minimum value.
-- @property min_value
-- @param number

--- The maximum value.
-- @property max_value
-- @param number

for _, prop in ipairs {"max_value", "min_value", "border_color", "color",
    "border_width", "paddings"} do
    radialprogressbar["set_"..prop] = function(self, value)
        self._private[prop] = value
        self:emit_signal("property::"..prop)
        self:emit_signal("widget::redraw_needed")
    end
    radialprogressbar["get_"..prop] = function(self)
        return self._private[prop] or beautiful["radialprogressbar_"..prop]
    end
end

function radialprogressbar:set_paddings(val)
    self._private.paddings = type(val) == "number" and {
        left   = val,
        right  = val,
        top    = val,
        bottom = val,
    } or val or {}
    self:emit_signal("property::paddings")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
end

--- Returns a new radialprogressbar layout. A radialprogressbar layout
-- radialprogressbars a given widget. Use `.widget` to set the widget.
-- @param[opt] widget The widget to display.
-- @constructorfct wibox.container.radialprogressbar
local function new(widget)
    local ret = base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(ret, radialprogressbar)
    ret._private.max_value = 1
    ret._private.min_value = 0

    ret:set_widget(widget)

    return ret
end

function radialprogressbar.mt:__call(_, ...)
    return new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(radialprogressbar, radialprogressbar.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
