---------------------------------------------------------------------------
-- Management of widget hierarchies. Each widget hierarchy object has a widget
-- for which it saves e.g. size and transformation in its parent. Also, each
-- widget has a number of children.
--
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
-- @module wibox.hierarchy
---------------------------------------------------------------------------

local matrix = require("gears.matrix")
local protected_call = require("gears.protected_call")
local cairo = require("lgi").cairo
local base = require("wibox.widget.base")
local no_parent = base.no_parent_I_know_what_I_am_doing

local hierarchy = {}

local widgets_to_count = setmetatable({}, { __mode = "k" })

--- Add a widget to the list of widgets for which hierarchies should count their
-- occurrences. Note that for correct operations, the widget must not yet be
-- visible in any hierarchy.
-- @param widget The widget that should be counted.
function hierarchy.count_widget(widget)
    widgets_to_count[widget] = true
end

local function hierarchy_new(redraw_callback, layout_callback, callback_arg)
    local result = {
        _matrix = matrix.identity,
        _matrix_to_device = matrix.identity,
        _need_update = true,
        _widget = nil,
        _context = nil,
        _redraw_callback = redraw_callback,
        _layout_callback = layout_callback,
        _callback_arg = callback_arg,
        _size = {
            width = nil,
            height = nil
        },
        _draw_extents = {
            x = 0,
            y = 0,
            width = 0,
            height = 0
        },
        _parent = nil,
        _children = {},
        _widget_counts = {},
    }

    function result._redraw()
        redraw_callback(result, callback_arg)
    end
    function result._layout()
        local h = result
        while h do
            h._need_update = true
            h = h._parent
        end
        layout_callback(result, callback_arg)
    end
    function result._emit_recursive(widget, name, ...)
        local cur = result
        assert(widget == cur._widget)
        while cur do
            if cur._widget then
                cur._widget:emit_signal(name, ...)
            end
            cur = cur._parent
        end
    end

    for k, f in pairs(hierarchy) do
        if type(f) == "function" then
            result[k] = f
        end
    end
    return result
end

local hierarchy_update
function hierarchy_update(self, context, widget, width, height, region, matrix_to_parent, matrix_to_device)
    if (not self._need_update) and self._widget == widget and
            self._context == context and
            self._size.width == width and self._size.height == height and
            matrix.equals(self._matrix, matrix_to_parent) and
            matrix.equals(self._matrix_to_device, matrix_to_device) then
        -- Nothing changed
        return
    end

    self._need_update = false

    local old_x, old_y, old_width, old_height
    local old_widget = self._widget
    if self._size.width and self._size.height then
        local x, y, w, h = matrix.transform_rectangle(self._matrix_to_device, 0, 0, self._size.width, self._size.height)
        old_x, old_y = math.floor(x), math.floor(y)
        old_width, old_height = math.ceil(x + w) - old_x, math.ceil(y + h) - old_y
    else
        old_x, old_y, old_width, old_height = 0, 0, 0, 0
    end

    -- Disconnect old signals
    if old_widget and old_widget ~= widget then
        self._widget:disconnect_signal("widget::redraw_needed", self._redraw)
        self._widget:disconnect_signal("widget::layout_changed", self._layout)
        self._widget:disconnect_signal("widget::emit_recursive", self._emit_recursive)
    end

    -- Save the arguments we need to save
    self._widget = widget
    self._context = context
    self._size.width = width
    self._size.height = height
    self._matrix = matrix_to_parent
    self._matrix_to_device = matrix_to_device

    -- Connect signals
    if old_widget ~= widget then
        widget:weak_connect_signal("widget::redraw_needed", self._redraw)
        widget:weak_connect_signal("widget::layout_changed", self._layout)
        widget:weak_connect_signal("widget::emit_recursive", self._emit_recursive)
    end

    -- Update children
    local old_children = self._children
    local layout_result = base.layout_widget(no_parent, context, widget, width, height)
    self._children = {}
    for _, w in ipairs(layout_result or {}) do
        local r = table.remove(old_children, 1)
        if not r then
            r = hierarchy_new(self._redraw_callback, self._layout_callback, self._callback_arg)
            r._parent = self
        end
        hierarchy_update(r, context, w._widget, w._width, w._height, region, w._matrix, w._matrix * matrix_to_device)
        table.insert(self._children, r)
    end

    -- Calculate the draw extents
    local x1, y1, x2, y2 = 0, 0, width, height
    for _, h in ipairs(self._children) do
        local px, py, pwidth, pheight = matrix.transform_rectangle(h._matrix, h:get_draw_extents())
        x1 = math.min(x1, px)
        y1 = math.min(y1, py)
        x2 = math.max(x2, px + pwidth)
        y2 = math.max(y2, py + pheight)
    end
    self._draw_extents = {
        x = x1, y = y1,
        width = x2 - x1,
        height = y2 - y1
    }

    -- Update widget counts
    self._widget_counts = {}
    if widgets_to_count[widget] and width > 0 and height > 0 then
        self._widget_counts[widget] = 1
    end
    for _, h in ipairs(self._children) do
        for w, count in pairs(h._widget_counts) do
            self._widget_counts[w] = (self._widget_counts[w] or 0) + count
        end
    end

    -- Check which part needs to be redrawn

    -- Are there any children which were removed? Their area needs a redraw.
    for _, child in ipairs(old_children) do
        local x, y, w, h = matrix.transform_rectangle(child._matrix_to_device, child:get_draw_extents())
        region:union_rectangle(cairo.RectangleInt{
            x = x, y = y, width = w, height = h
        })
        child._parent = nil
    end

    -- Did we change and need to be redrawn?
    local x, y, w, h = matrix.transform_rectangle(self._matrix_to_device, 0, 0, self._size.width, self._size.height)
    local new_x, new_y = math.floor(x), math.floor(y)
    local new_width, new_height = math.ceil(x + w) - new_x, math.ceil(y + h) - new_y
    if new_x ~= old_x or new_y ~= old_y or new_width ~= old_width or new_height ~= old_height or
            widget ~= old_widget then
        region:union_rectangle(cairo.RectangleInt{
            x = old_x, y = old_y, width = old_width, height = old_height
        })
        region:union_rectangle(cairo.RectangleInt{
            x = new_x, y = new_y, width = new_width, height = new_height
        })
    end
end

--- Create a new widget hierarchy that has no parent.
-- @param context The context in which we are laid out.
-- @param widget The widget that is at the base of the hierarchy.
-- @param width The available width for this hierarchy.
-- @param height The available height for this hierarchy.
-- @param redraw_callback Callback that is called with the corresponding widget
--   hierarchy on widget::redraw_needed on some widget.
-- @param layout_callback Callback that is called with the corresponding widget
--   hierarchy on widget::layout_changed on some widget.
-- @param callback_arg A second argument that is given to the above callbacks.
-- @return A new widget hierarchy
function hierarchy.new(context, widget, width, height, redraw_callback, layout_callback, callback_arg)
    local result = hierarchy_new(redraw_callback, layout_callback, callback_arg)
    result:update(context, widget, width, height)
    return result
end

--- Update a widget hierarchy with some new state.
-- @param context The context in which we are laid out.
-- @param widget The widget that is at the base of the hierarchy.
-- @param width The available width for this hierarchy.
-- @param height The available height for this hierarchy.
-- @param[opt] region A region to use for accumulating changed parts
-- @return A cairo region describing the changed parts (either the `region`
--   argument or a new, internally created region).
function hierarchy:update(context, widget, width, height, region)
    region = region or cairo.Region.create()
    hierarchy_update(self, context, widget, width, height, region, self._matrix, self._matrix_to_device)
    return region
end

--- Get the widget that this hierarchy manages.
function hierarchy:get_widget()
    return self._widget
end

--- Get a matrix that transforms to the parent's coordinate space from this
-- hierarchy's coordinate system.
-- @return A matrix describing the transformation.
function hierarchy:get_matrix_to_parent()
    return self._matrix
end

--- Get a matrix that transforms to the base of this hierarchy's coordinate
-- system (aka the coordinate system of the device that this
-- hierarchy is applied upon) from this hierarchy's coordinate system.
-- @return A matrix describing the transformation.
function hierarchy:get_matrix_to_device()
    return self._matrix_to_device
end

--- Get a matrix that transforms from the parent's coordinate space into this
-- hierarchy's coordinate system.
-- @return A matrix describing the transformation.
function hierarchy:get_matrix_from_parent()
    local m = self:get_matrix_to_parent()
    return m:invert()
end

--- Get a matrix that transforms from the base of this hierarchy's coordinate
-- system (aka the coordinate system of the device that this
-- hierarchy is applied upon) into this hierarchy's coordinate system.
-- @return A matrix describing the transformation.
function hierarchy:get_matrix_from_device()
    local m = self:get_matrix_to_device()
    return m:invert()
end

--- Get the extents that this hierarchy possibly draws to (in the current coordinate space).
-- This includes the size of this element plus the size of all children
-- (after applying the corresponding transformation).
-- @return x, y, width, height
function hierarchy:get_draw_extents()
    local ext = self._draw_extents
    return ext.x, ext.y, ext.width, ext.height
end

--- Get the size that this hierarchy logically covers (in the current coordinate space).
-- @return width, height
function hierarchy:get_size()
    local ext = self._size
    return ext.width, ext.height
end

--- Get a list of all children.
-- @return List of all children hierarchies.
function hierarchy:get_children()
    return self._children
end

--- Count how often this widget is visible inside this hierarchy. This function
-- only works with widgets registered via `count_widget`.
-- @param widget The widget that should be counted
-- @return The number of times that this widget is contained in this hierarchy.
function hierarchy:get_count(widget)
    return self._widget_counts[widget] or 0
end

--- Does the given cairo context have an empty clip (aka "no drawing possible")?
local function empty_clip(cr)
    local _, _, width, height = cr:clip_extents()
    return width == 0 or height == 0
end

--- Draw a hierarchy to some cairo context.
-- This function draws the widgets in this widget hierarchy to the given cairo
-- context. The context's clip is used to skip parts that aren't visible.
-- @param context The context in which widgets are drawn.
-- @param cr The cairo context that is used for drawing.
function hierarchy:draw(context, cr)
    local widget = self:get_widget()
    if not widget._private.visible then
        return
    end

    cr:save()
    cr:transform(self:get_matrix_to_parent():to_cairo_matrix())

    -- Clip to the draw extents
    cr:rectangle(self:get_draw_extents())
    cr:clip()

    -- Draw if needed
    if not empty_clip(cr) then
        local opacity = widget:get_opacity()
        local function call(func, extra_arg1, extra_arg2)
            if not func then return end
            if not extra_arg2 then
                protected_call(func, widget, context, cr, self:get_size())
            else
                protected_call(func, widget, context, extra_arg1, extra_arg2, cr, self:get_size())
            end
        end

        -- Prepare opacity handling
        if opacity ~= 1 then
            cr:push_group()
        end

        -- Draw the widget
        cr:save()
        cr:rectangle(0, 0, self:get_size())
        cr:clip()
        call(widget.draw)
        cr:restore()

        -- Draw its children (We already clipped to the draw extents above)
        call(widget.before_draw_children)
        for i, wi in ipairs(self:get_children()) do
            call(widget.before_draw_child, i, wi:get_widget())
            wi:draw(context, cr)
            call(widget.after_draw_child, i, wi:get_widget())
        end
        call(widget.after_draw_children)

        -- Apply opacity
        if opacity ~= 1 then
            cr:pop_group_to_source()
            cr.operator = cairo.Operator.OVER
            cr:paint_with_alpha(opacity)
        end
    end

    cr:restore()
end

return hierarchy

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
