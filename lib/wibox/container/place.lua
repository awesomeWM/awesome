---------------------------------------------------------------------------
-- A container used to place smaller widgets into larger space.
--
--@DOC_wibox_container_defaults_place_EXAMPLE@
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @containermod wibox.container.place
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local setmetatable = setmetatable
local base = require("wibox.widget.base")
local gtable = require("gears.table")

local place = { mt = {} }

-- Take the widget width/height and compute the position from the full
-- width/height
local align_fct = {
    left   = function(_  , _   ) return 0                         end,
    center = function(wdg, orig) return math.max(0, (orig-wdg)/2) end,
    right  = function(wdg, orig) return math.max(0, orig-wdg    ) end,
}
align_fct.top, align_fct.bottom = align_fct.left, align_fct.right

-- Shared with some subclasses like the `tiled` and `scaled` modules.
function place:_layout(context, width, height)
    local w, h = base.fit_widget(self, context, self._private.widget, width, height)

    if self._private.content_fill_horizontal then
        w = width
    end

    if self._private.content_fill_vertical then
        h = height
    end

    local valign = self._private.valign or "center"
    local halign = self._private.halign or "center"

    local x, y = align_fct[halign](w, width), align_fct[valign](h, height)

    -- Sub pixels makes everything blurry. This is now what people expect.
    x, y = math.floor(x), math.floor(y)

    return x, y, w, h
end

-- Layout this layout
function place:layout(context, width, height)

    if not self._private.widget then
        return
    end

    local x, y, w, h = self:_layout(context, width, height)

    return { base.place_widget_at(self._private.widget, x, y, w, h) }
end

-- Fit this layout into the given area
function place:fit(context, width, height)
    if not self._private.widget then
        return 0, 0
    end

    local w, h = base.fit_widget(self, context, self._private.widget, width, height)

    return (self._private.fill_horizontal or self._private.content_fill_horizontal)
        and width or w,
    (self._private.fill_vertical or self._private.content_fill_vertical)
        and height or h
end

--- The widget to be placed.
--
-- @property widget
-- @tparam widget widget The widget
-- @interface container

place.set_widget = base.set_widget_common

function place:get_widget()
    return self._private.widget
end

function place:get_children()
    return {self._private.widget}
end

function place:set_children(children)
    self:set_widget(children[1])
end

--- Reset this layout. The widget will be removed and the rotation reset.
-- @method reset
-- @noreturn
-- @interface container
function place:reset()
    self:set_widget(nil)
end

--- The vertical alignment.
--
-- Possible values are:
--
-- * *top*
-- * *center* (default)
-- * *bottom*
--
--@DOC_wibox_container_place_valign_EXAMPLE@
--
-- @property valign
-- @tparam[opt="center"] string valign
-- @propemits true false

--- The horizontal alignment.
--
-- Possible values are:
--
-- * *left*
-- * *center* (default)
-- * *right*
--
--@DOC_wibox_container_place_halign_EXAMPLE@
--
-- @property halign
-- @tparam[opt="center"] string halign
-- @propemits true false

function place:set_valign(value)
    if value ~= "center" and value ~= "top" and value ~= "bottom" then
        return
    end

    self._private.valign = value
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::valign", value)
end

function place:set_halign(value)
    if value ~= "center" and value ~= "left" and value ~= "right" then
        return
    end

    self._private.halign = value
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::halign", value)
end

--- Fill the vertical space.
--
-- @property fill_vertical
-- @tparam[opt=false] boolean fill_vertical
-- @propemits true false

function place:set_fill_vertical(value)
    self._private.fill_vertical = value
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::fill_vertical", value)
end

--- Fill the horizontal space.
--
-- @property fill_horizontal
-- @tparam[opt=false] boolean fill_horizontal
-- @propemits true false

function place:set_fill_horizontal(value)
    self._private.fill_horizontal = value
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::fill_horizontal", value)
end

--- Stretch the contained widget so it takes all the vertical space.
--
--@DOC_wibox_container_place_content_fill_vertical_EXAMPLE@
--
-- @property content_fill_vertical
-- @tparam[opt=false] boolean content_fill_vertical
-- @propemits true false

function place:set_content_fill_vertical(value)
    self._private.content_fill_vertical = value
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::content_fill_vertical", value)
end

--- Stretch the contained widget so it takes all the horizontal space.
--
--@DOC_wibox_container_place_content_fill_horizontal_EXAMPLE@
--
-- @property content_fill_horizontal
-- @tparam[opt=false] boolean content_fill_horizontal
-- @propemits true false

function place:set_content_fill_horizontal(value)
    self._private.content_fill_horizontal = value
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::content_fill_horizontal", value)
end

--- Returns a new place container.
--
-- @tparam[opt] widget widget The widget to display.
-- @tparam[opt="center"] string halign The horizontal alignment
-- @tparam[opt="center"] string valign The vertical alignment
-- @treturn table A new place container.
-- @constructorfct wibox.container.place
local function new(widget, halign, valign)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, place, true)

    ret:set_widget(widget)
    ret:set_halign(halign)
    ret:set_valign(valign)

    return ret
end

function place.mt:__call(...)
    return new(...)
end

return setmetatable(place, place.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
