---------------------------------------------------------------------------
-- Management of widget hierarchies. Each widget hierarchy object has a widget
-- for which it saves e.g. size and transformation in its parent. Also, each
-- widget has a number of children.
--
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @module wibox.hierarchy
---------------------------------------------------------------------------

local matrix = require("gears.matrix")
local cairo = require("lgi").cairo

local hierarchy = {}

--- Create a new widget hierarchy that has no parent.
-- @param context The context in which we are laid out.
-- @param widget The widget that is at the base of the hierarchy.
-- @param width The available width for this hierarchy
-- @param height The available height for this hierarchy
-- @param redraw_callback Callback that is called with the corresponding widget
--        hierarchy on widget::redraw_needed on some widget.
-- @param layout_callback Callback that is called with the corresponding widget
--        hierarchy on widget::layout_changed on some widget.
-- @param root The root of the widget hierarchy or nil if this creates the root.
-- @return A new widget hierarchy
local function hierarchy_new(context, widget, width, height, redraw_callback, layout_callback, root)
    local children = widget._layout_cache:get(context, width, height)
    local draws_x1, draws_y1, draws_x2, draws_y2 = 0, 0, width, height
    local result = {
        _parent = nil,
        _root = nil,
        _matrix = cairo.Matrix.create_identity(),
        _widget = widget,
        _size = {
            width = width,
            height = height
        },
        _draw_extents = nil,
        _children = {}
    }

    result._root = root or result
    result._redraw = function() redraw_callback(result) end
    result._layout = function() layout_callback(result) end
    widget:weak_connect_signal("widget::redraw_needed", result._redraw)
    widget:weak_connect_signal("widget::layout_changed", result._layout)

    for _, w in ipairs(children or {}) do
        local r = hierarchy_new(context, w._widget, w._width, w._height, redraw_callback, layout_callback, result._root)
        r._matrix = w._matrix
        r._parent = result
        table.insert(result._children, r)

        -- Update our drawing extents
        local s = r._draw_extents
        local px, py, pwidth, pheight = matrix.transform_rectangle(r._matrix,
                s.x, s.y, s.width, s.height)
        local px2, py2 = px + pwidth, py + pheight
        draws_x1 = math.min(draws_x1, px)
        draws_y1 = math.min(draws_y1, py)
        draws_x2 = math.max(draws_x2, px2)
        draws_y2 = math.max(draws_y2, py2)
    end
    result._draw_extents = {
        x = draws_x1,
        y = draws_y1,
        width = draws_x2 - draws_x1,
        height = draws_y2 - draws_y1
    }

    for k, f in pairs(hierarchy) do
        if type(f) == "function" then
            result[k] = f
        end
    end
    return result
end

--- Create a new widget hierarchy that has no parent.
-- @param context The context in which we are laid out.
-- @param widget The widget that is at the base of the hierarchy.
-- @param width The available width for this hierarchy
-- @param height The available height for this hierarchy
-- @param redraw_callback Callback that is called with the corresponding widget
--        hierarchy on widget::redraw_needed on some widget.
-- @param layout_callback Callback that is called with the corresponding widget
--        hierarchy on widget::layout_changed on some widget.
-- @return A new widget hierarchy
function hierarchy.new(context, widget, width, height, redraw_callback, layout_callback)
    return hierarchy_new(context, widget, width, height, redraw_callback, layout_callback, nil)
end

--- Get the parent hierarchy of this widget hierarchy (or nil).
function hierarchy:get_parent()
    return self._parent
end

--- Get the root of this widget hierarchy.
function hierarchy:get_root()
    return self._root
end

--- Get the widget that this hierarchy manages.
function hierarchy:get_widget()
    return self._widget
end

--- Get a cairo matrix that transforms to the parent's coordinate space from
-- this hierarchy's coordinate system.
-- @return A cairo matrix describing the transformation.
function hierarchy:get_matrix_to_parent()
    return matrix.copy(self._matrix)
end

--- Get a cairo matrix that transforms to the base of this hierarchy's
-- coordinate system (aka the coordinate system of the device that this
-- hierarchy is applied upon) from this hierarchy's coordinate system.
-- @return A cairo matrix describing the transformation.
function hierarchy:get_matrix_to_device()
    if not self._matrix_to_device then
        local m = cairo.Matrix.create_identity()
        local state = self
        while state ~= nil do
            m:multiply(m, state._matrix)
            state = state._parent
        end
        self._matrix_to_device = m
    end
    return matrix.copy(self._matrix_to_device)
end

--- Get a cairo matrix that transforms from the parent's coordinate space into
-- this hierarchy's coordinate system.
-- @return A cairo matrix describing the transformation.
function hierarchy:get_matrix_from_parent()
    local m = self:get_matrix_to_parent()
    m:invert()
    return m
end

--- Get a cairo matrix that transforms from the base of this hierarchy's
-- coordinate system (aka the coordinate system of the device that this
-- hierarchy is applied upon) into this hierarchy's coordinate system.
-- @return A cairo matrix describing the transformation.
function hierarchy:get_matrix_from_device()
    local m = self:get_matrix_to_device()
    m:invert()
    return m
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

--- Compare two widget hierarchies and compute a cairo Region that contains all
-- rectangles that aren't the same between both hierarchies.
-- @param other The hierarchy to compare with
-- @return A cairo Region containing the differences.
function hierarchy:find_differences(other)
    local region = cairo.Region.create()
    local function needs_redraw(h)
        local m = h:get_matrix_to_device()
        local p = h._draw_extents
        local x, y, width, height = matrix.transform_rectangle(m, p.x, p.y, p.width, p.height)
        local x1, y1 = math.floor(x), math.floor(y)
        local x2, y2 = math.ceil(x + width), math.ceil(y + height)
        region:union_rectangle(cairo.RectangleInt({
            x = x1, y = y1, width = x2 - x1, height = y2 - y1
        }))
    end
    local compare
    compare = function(self, other)
        local s_size, o_size = self._size, other._size
        if s_size.width ~= o_size.width or s_size.height ~= o_size.height or
                #self._children ~= #other._children or self._widget ~= other._widget or
                not matrix.equals(self._matrix, other._matrix) then
            needs_redraw(self)
            needs_redraw(other)
        else
            for i = 1, #self._children do
                compare(self._children[i], other._children[i])
            end
        end
    end
    compare(self, other)
    return region
end

return hierarchy

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
