---------------------------------------------------------------------------
--- Place multiple widgets in multiple rows and columns.
--
-- Widgets spanning several columns or rows cannot be included using the
-- declarative system.
-- Instead, create the grid layout and call the `add_widget_at` method.
--
--@DOC_wibox_layout_grid_imperative_EXAMPLE@
--
-- The same can be done using the declarative syntax:
--
--@DOC_wibox_layout_grid_declarative1_EXAMPLE@
--
-- When `col_index` and `row_index` are not provided, the widgets are
-- automatically added next to each other spanning only one cell:
--
--@DOC_wibox_layout_grid_declarative2_EXAMPLE@
--
--@DOC_wibox_layout_defaults_grid_EXAMPLE@
-- @author getzze
-- @copyright 2017 getzze
-- @layoutmod wibox.layout.grid
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local setmetatable = setmetatable
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local table = table
local pairs = pairs
local ipairs = ipairs
local math = math
local gtable = require("gears.table")
local gmath = require("gears.math")
local gcolor = require("gears.color")
local base = require("wibox.widget.base")
local cairo = require("lgi").cairo

local grid = { mt = {} }

local properties = {
                    "orientation", "superpose",
                    "forced_num_rows", "forced_num_cols",
                    "min_cols_size", "min_rows_size",
                   }

local dir_properties = { "spacing", "homogeneous", "expand" }



--- Set the preferred orientation of the grid layout.
--
-- When calling `get_next_empty`, empty cells are browsed differently.
--
--@DOC_wibox_layout_grid_orientation_EXAMPLE@
-- @tparam[opt="vertical"] string orientation Preferred orientation.
-- @propertyvalue "horizontal" The grid can be extended horizontally. The current
--  column is filled first; if no empty cell is found up to `forced_num_rows`,
--  the next column is filled, creating it if it does not exist.
-- @propertyvalue "vertical" The grid can be extended vertically. The current row is
--  filled first; if no empty cell is found up to `forced_num_cols`, the next
--  row is filled, creating it if it does not exist.
-- @property orientation

--- Allow to superpose widgets in the same cell.
-- If false, check before adding a new widget if it will superpose with another
-- widget and prevent from adding it.
--
--@DOC_wibox_layout_grid_superpose_EXAMPLE@
-- @tparam[opt=false] boolean superpose
-- @property superpose

--- Force the number of rows of the layout.
-- @property forced_num_rows
-- @tparam[opt=nil] number|nil forced_num_rows
-- @propertytype nil Automatically determine the number of rows.
-- @propertyunit rows
-- @negativeallowed false
-- @see forced_num_cols
-- @see row_count

--- Force the number of columns of the layout.
-- @property forced_num_cols
-- @tparam[opt=nil] number|nil forced_num_cols
-- @propertytype nil Automatically determine the number of columns.'
-- @propertyunit columns
-- @negativeallowed false
-- @see forced_num_rows
-- @see column_count

--- Set the minimum size for the columns.
--
--@DOC_wibox_layout_grid_min_size_EXAMPLE@
-- @tparam[opt=0] number min_cols_size Minimum size of the columns.
-- @property min_cols_size
-- @propertyunit pixel
-- @negativeallowed false
-- @see min_rows_size

--- Set the minimum size for the rows.
-- @tparam[opt=0] number min_rows_size Minimum size of the rows.
-- @property min_rows_size
-- @propertyunit pixel
-- @negativeallowed false
-- @see min_cols_size

--- The spacing between columns.
--
-- @tparam[opt=0] number horizontal_spacing
-- @property horizontal_spacing
-- @propertyunit pixel
-- @negativeallowed false
-- @see spacing
-- @see vertical_spacing

--- The spacing between rows.
--
-- @tparam[opt=0] number vertical_spacing
-- @property vertical_spacing
-- @propertyunit pixel
-- @negativeallowed false
-- @see spacing
-- @see horizontal_spacing

--- The spacing between rows and columns.
--
-- Get the value `horizontal_spacing` or `vertical_spacing` defined by the
-- preferred `orientation`.
--
--@DOC_wibox_layout_grid_spacing_EXAMPLE@
--
-- When a border is present, the spacing is applied on both side of the border,
-- thus is twice as large:
--
-- @DOC_wibox_layout_grid_border_width3_EXAMPLE@
--
-- @tparam[opt=0] number spacing
-- @property spacing
-- @negativeallowed false
-- @see vertical_spacing
-- @see horizontal_spacing

--- Controls if the columns are expanded to use all the available width.
--
-- @tparam[opt=false] boolean horizontal_expand Expand the grid into the available space
-- @property horizontal_expand
-- @see expand
-- @see vertical_expand

--- Controls if the rows are expanded to use all the available height.
--
-- @tparam[opt=false] boolean vertical_expand Expand the grid into the available space
-- @property vertical_expand
-- @see expand
-- @see horizontal_expand

--- Controls if the columns/rows are expanded to use all the available space.
--
-- Get the value `horizontal_expand` or `vertical_expand` defined by the
-- preferred `orientation`.
--
--@DOC_wibox_layout_grid_expand_EXAMPLE@
-- @tparam[opt=false] boolean expand Expand the grid into the available space
-- @property expand
-- @see horizontal_expand
-- @see vertical_expand

--- Controls if the columns all have the same width or if the width of each
-- column depends on the content.
--
-- see `homogeneous`
--
-- @tparam[opt=true] boolean horizontal_homogeneous All the columns have the same width.
-- @property horizontal_homogeneous
-- @see vertical_homogeneous
-- @see homogeneous

--- Controls if the rows all have the same height or if the height of each row
-- depends on the content.
--
-- see `homogeneous`
--
-- @tparam[opt=true] boolean vertical_homogeneous All the rows have the same height.
-- @property vertical_homogeneous
-- @see homogeneous
-- @see horizontal_homogeneous

--- Controls if the columns/rows all have the same size or if the size depends
-- on the content.
-- Set both `horizontal_homogeneous` and `vertical_homogeneous` to the same value.
-- Get the value `horizontal_homogeneous` or `vertical_homogeneous` defined
-- by the preferred `orientation`.
--
--@DOC_wibox_layout_grid_expand_EXAMPLE@
-- @tparam[opt=true] boolean homogeneous All the columns/rows have the same size.
-- @property homogeneous
-- @see vertical_homogeneous
-- @see horizontal_homogeneous

--- The number of rows.
--
-- If `forced_num_rows` is set, then its value is returned, otherwise it will
-- return the maximum actual number of widgets in a row.
--
-- @property row_count
-- @tparam integer row_count
-- @readonly
-- @see forced_num_rows

--- The number of columns.
--
-- If `forced_num_cols` is set, then its value is returned, otherwise it will
-- return the maximum actual number of widgets in a column.
--
-- @property column_count
-- @readonly
-- @tparam integer column_count
-- @see forced_num_cols

--- Child widget position. Return of `get_widget_position`.
-- @field row Top row index
-- @field col Left column index
-- @field row_span Number of rows to span
-- @field col_span Number of columns to span
-- @table position

-- Return the maximum value of a table.
local function max_value(t)
    local m = 0
    for _,v in ipairs(t) do
        if m < v then m = v end
    end
    return m
end

-- Return the sum of the values in the table.
local function sum_values(t)
    local m = 0
    for _,v in ipairs(t) do
        m = m + v
    end
    return m
end


-- Find a widget in a widget_table, by matching the coordinates.
-- Using the  `row`:`col` coordinates, and the spans `row_span` and `col_span`
-- @tparam table widgets_table Table of the widgets present in the grid
-- @tparam number row Row number for the top left corner of the widget
-- @tparam number col Column number for the top left corner of the widget
-- @tparam number row_span The number of rows the widget spans (default to 1)
-- @tparam number col_span The number of columns the widget spans (default to 1)
-- @treturn table Table of index of widget_table
local function find_widgets_at(widgets_table, row, col, row_span, col_span)
    if not row or row < 1 or not col or col < 1 then return nil end
    row_span = (row_span and row_span > 0) and row_span or 1
    col_span = (col_span and col_span > 0) and col_span or 1
    local ret = {}
    for index, data in ipairs(widgets_table) do
        -- If one rectangular widget is on left side of other
        local test_horizontal = not (row > data.row + data.row_span - 1
            or data.row > row + row_span - 1)
        -- If one rectangular widget is above other
        local test_vertical   = not (col > data.col + data.col_span - 1
            or data.col > col + col_span - 1)
        if test_horizontal and test_vertical then
            table.insert(ret, index)
        end
    end
    -- reverse sort for safe removal of indices
    table.sort(ret, function(a,b) return a>b end)
    return #ret > 0 and ret or nil
end


-- Find a widget in a widget_table, by matching the object.
-- @tparam table widgets_table Table of the widgets present in the grid
-- @param widget The widget to find
-- @treturn number|nil The index of the widget in widget_table, `nil` if not found
local function find_widget(widgets_table, widget)
    for index, data in ipairs(widgets_table) do
        if data.widget == widget then
            return index
        end
    end
    return nil
end

--- Get the number of rows and columns occupied by the widgets in the grid.
-- @method get_dimension
-- @treturn number,number The number of rows and columns
function grid:get_dimension()
    return self._private.num_rows, self._private.num_cols
end

-- Update the number of rows and columns occupied by the widgets in the grid.
local function update_dimension(self)
    local num_rows, num_cols = 0, 0
    if self._private.forced_num_rows then
        num_rows = self._private.forced_num_rows
    end
    if self._private.forced_num_cols then
        num_cols = self._private.forced_num_cols
    end

    for _, data in ipairs(self._private.widgets) do
        num_rows = math.max(num_rows, data.row + data.row_span - 1)
        num_cols = math.max(num_cols, data.col + data.col_span - 1)
    end
    self._private.num_rows = num_rows
    self._private.num_cols = num_cols
end


--- Find the next available cell to insert a widget.
-- The grid is browsed according to the `orientation`.
-- @method get_next_empty
-- @tparam[opt=1] number hint_row The row coordinate of the last occupied cell.
-- @tparam[opt=1] number hint_column The column coordinate of the last occupied cell.
-- @return number,number The row,column coordinate of the next empty cell
function grid:get_next_empty(hint_row, hint_column)
    local row    = (hint_row and hint_row > 0) and hint_row or 1
    local column = (hint_column and hint_column > 0) and hint_column or 1

    local next_field
    if self._private.orientation == "vertical" then
        next_field = function(x, y)
            if y < self._private.num_cols then
                return x, y+1
            end
            return x+1,1
        end
    elseif self._private.orientation == "horizontal" then
        next_field = function(x, y)
            if x < self._private.num_rows then
                return x+1, y
            end
            return 1,y+1
        end
    end
    while true do
        if find_widgets_at(self._private.widgets, row, column, 1, 1) == nil then
            return row, column
        end
        row, column = next_field(row, column)
    end
end


--- Add some widgets to the given grid layout.
--
-- The widgets are assumed to span one cell.
--
-- If the widgets have a `row_index`, `col_index`, `col_span`
-- or `row_span` property, it will be honored.
--
-- @method add
-- @tparam wibox.widget ... Widgets that should be added (must at least be one)
-- @interface layout
-- @noreturn
function grid:add(...)
    local args = { n=select('#', ...), ... }
    assert(args.n > 0, "need at least one widget to add")
    local row, column
    for i=1, args.n do
        local w = args[i]
        -- Get the next empty coordinate to insert the widget
        row, column = self:get_next_empty(row, column)
        self:add_widget_at(
            w,
            w.row_index or row,
            w.col_index or column,
            w.row_span or 1,
            w.col_span or 1
        )
    end
end

--- Add a widget to the grid layout at specific coordinate.
--
--@DOC_wibox_layout_grid_add_EXAMPLE@
--
-- @method add_widget_at
-- @tparam wibox.widget child Widget that should be added
-- @tparam number row Row number for the top left corner of the widget
-- @tparam number col Column number for the top left corner of the widget
-- @tparam[opt=1] number row_span The number of rows the widget spans.
-- @tparam[opt=1] number col_span The number of columns the widget spans.
-- @treturn boolean index If the operation is successful
function grid:add_widget_at(child, row, col, row_span, col_span)
    if not row or row < 1 or not col or col < 1 then return false end
    row_span = (row_span and row_span > 0) and row_span or 1
    col_span = (col_span and col_span > 0) and col_span or 1

    -- check if the object is a widget
    child = base.make_widget_from_value(child)
    base.check_widget(child)

    -- test if the new widget superpose with existing ones
    local superpose = find_widgets_at(
        self._private.widgets, row, col, row_span, col_span
    )

    if not self._private.superpose and superpose then
        return false
    end

    -- Add grid information attached to the widget
    local child_data = {
        widget = child,
        row = row,
        col = col,
        row_span = row_span,
        col_span = col_span
    }
    table.insert(self._private.widgets, child_data)

    -- Update the row and column numbers
    self._private.num_rows = math.max(self._private.num_rows, row + row_span - 1)
    self._private.num_cols = math.max(self._private.num_cols, col + col_span - 1)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    return true
end

--- Remove one or more widgets from the layout.
-- @method remove
-- @param ... Widgets that should be removed (must at least be one)
-- @treturn boolean If the operation is successful
function grid:remove(...)
    local args = { ... }
    local ret = false
    for _, rem_widget in ipairs(args) do
        local index = find_widget(self._private.widgets, rem_widget)
        if index ~= nil then
            table.remove(self._private.widgets, index)
            ret = true
        end
    end
    if ret then
        -- Recalculate num_rows and num_cols
        update_dimension(self)
        self:emit_signal("widget::layout_changed")
        self:emit_signal("widget::redraw_needed")
    end
    return ret
end


--- Remove widgets at the coordinates.
--
--@DOC_wibox_layout_grid_remove_EXAMPLE@
--
-- @method remove_widgets_at
-- @tparam number row The row coordinate of the widget to remove
-- @tparam number col The column coordinate of the widget to remove
-- @tparam[opt=1] number row_span The number of rows the area to remove spans.
-- @tparam[opt=1] number col_span The number of columns the area to remove spans.
-- @treturn boolean If the operation is successful (widgets found)
function grid:remove_widgets_at(row, col, row_span, col_span)
    local widget_indices = find_widgets_at(
        self._private.widgets, row, col, row_span, col_span
    )
    if widget_indices == nil then return false end

    for _,index in ipairs(widget_indices) do
        table.remove(self._private.widgets, index)
    end
    -- Recalculate num_rows and num_cols
    update_dimension(self)
    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    return true
end

--- Return the coordinates of the widget.
-- @method get_widget_position
-- @tparam widget widget The widget
-- @treturn table The `position` table of the coordinates in the grid, with
-- fields `row`, `col`, `row_span` and `col_span`.
function grid:get_widget_position(widget)
    local index = find_widget(self._private.widgets, widget)
    if index == nil then return nil end
    local data = self._private.widgets[index]
    local ret = {}
    ret["row"] = data.row
    ret["col"] = data.col
    ret["row_span"] = data.row_span
    ret["col_span"] = data.col_span
    return ret
end


--- Return the widgets at the coordinates.
-- @method get_widgets_at
-- @tparam number row The row coordinate of the widget
-- @tparam number col The column coordinate of the widget
-- @tparam[opt=1] number row_span The number of rows to span.
-- @tparam[opt=1] number col_span The number of columns to span.
-- @treturn table The widget(s) found at the specific coordinates, nil if no widgets found
function grid:get_widgets_at(row, col, row_span, col_span)
    local widget_indices = find_widgets_at(
        self._private.widgets, row, col, row_span, col_span
    )

    if widget_indices == nil then return nil end

    local ret = {}
    for _,index in ipairs(widget_indices) do
        local data = self._private.widgets[index]
        table.insert(ret, data.widget)
    end

    return #ret > 0 and ret or nil
end

--- Replace old widget by new widget, spanning the same columns and rows.
-- @method replace_widget
-- @tparam widget old The widget to remove
-- @tparam widget new The widget to add
-- @treturn boolean If the operation is successful (widget found)
function grid:replace_widget(old, new)
    -- check if the new object is a widget
    local status = pcall(function () base.check_widget(new) end)
    if not status then return false end
    -- find the old widget
    local index = find_widget(self._private.widgets, old)
    if index == nil then return false end

    -- get old widget position
    local data = self._private.widgets[index]
    local row, col, row_span, col_span = data.row, data.col, data.row_span, data.col_span

    table.remove(self._private.widgets, index)
    return self:add_widget_at(new, row, col, row_span, col_span)
end

-- Update position of the widgets when inserting, adding or removing a row or a column.
-- @tparam table table_widgets Table of widgets
-- @tparam string orientation Orientation of the line: "horizontal" -> column, "vertical" -> row
-- @tparam number index Index of the line
-- @tparam string mode insert, extend or remove
-- @tparam boolean after Add the line after the index instead of inserting it before.
-- @tparam boolean extend Extend the line at index instead of inserting an empty line.
local function update_widgets_position(table_widgets, orientation, index, mode)
    local t = orientation == "horizontal" and "col" or "row"
    local to_remove = {}
    -- inc   : Index increment or decrement
    -- first : Offset index for top-left cell of the widgets to shift
    -- last  : Offset index for bottom-right cell of the widgets to resize
    local inc, first, last
    if mode == "remove" then
        inc, first, last = -1, 1, 1
    elseif mode == "insert" then
        inc, first, last = 1, 0, 0
    elseif mode == "extend" then
        inc, first, last = 1, 1, 0
    else
        return
    end
    for i, data in ipairs(table_widgets) do
        -- single widget in the line
        if mode == "remove" and data[t] == index and data[t .. "_span"] == 1 then
            table.insert(to_remove, i)
        -- widgets to shift
        elseif data[t] >= index + first then
            data[t] = data[t] + inc
        -- widgets to resize
        elseif data[t] + data[t .. "_span"] - 1 >= index + last then
            data[t .. "_span"] = data[t .. "_span"] + inc
        end
    end
    if mode == "remove" then
        -- reverse sort to remove
        table.sort(to_remove, function(a,b) return a>b end)
        -- Remove widgets
        for _,i in ipairs(to_remove) do
            table.remove(table_widgets, i)
        end
    end
end

--- Insert column at index.
--
--@DOC_wibox_layout_grid_insert_column_EXAMPLE@
--
-- @method insert_column
-- @tparam number|nil index Insert the new column at index. If `nil`, the column is added at the end.
-- @treturn number The index of the inserted column
function grid:insert_column(index)
    if index == nil or index > self._private.num_cols + 1 or index < 1 then
        index = self._private.num_cols + 1
    end
    -- Update widget positions
    update_widgets_position(self._private.widgets, "horizontal", index, "insert")
    -- Recalculate number of rows and columns
    self._private.num_cols = self._private.num_cols + 1
    return index
end

--- Extend column at index.
--@DOC_wibox_layout_grid_extend_column_EXAMPLE@
--
-- @method extend_column
-- @tparam number|nil index Extend the column at index. If `nil`, the last column is extended.
-- @treturn number The index of the extended column
function grid:extend_column(index)
    if index == nil or index > self._private.num_cols or index < 1 then
        index = self._private.num_cols
    end
    -- Update widget positions
    update_widgets_position(self._private.widgets, "horizontal", index, "extend")
    -- Recalculate number of rows and columns
    self._private.num_cols = self._private.num_cols + 1
    return index
end

--- Remove column at index.
--
--@DOC_wibox_layout_grid_remove_column_EXAMPLE@
--
-- @method remove_column
-- @tparam number|nil index Remove column at index. If `nil`, the last column is removed.
-- @treturn number The index of the removed column
function grid:remove_column(index)
    if index == nil or index > self._private.num_cols or index < 1 then
        index = self._private.num_cols
    end
    -- Update widget positions
    update_widgets_position(self._private.widgets, "horizontal", index, "remove")
    -- Recalculate number of rows and columns
    update_dimension(self)
    return index
end

--- Insert row at index.
--
-- see `insert_column`
--
-- @method insert_row
-- @tparam number|nil index Insert the new row at index. If `nil`, the row is added at the end.
-- @treturn number The index of the inserted row
function grid:insert_row(index)
    if index == nil or index > self._private.num_rows + 1 or index < 1 then
        index = self._private.num_rows + 1
    end
    -- Update widget positions
    update_widgets_position(self._private.widgets, "vertical", index, "insert")
    -- Recalculate number of rows and columns
    self._private.num_rows = self._private.num_rows + 1
    return index
end

--- Extend row at index.
--
-- see `extend_column`
--
-- @method extend_row
-- @tparam number|nil index Extend the row at index. If `nil`, the last row is extended.
-- @treturn number The index of the extended row
function grid:extend_row(index)
    if index == nil or index > self._private.num_rows or index < 1 then
        index = self._private.num_rows
    end
    -- Update widget positions
    update_widgets_position(self._private.widgets, "vertical", index, "extend")
    -- Recalculate number of rows and columns
    self._private.num_rows = self._private.num_rows + 1
    return index
end

--- Remove row at index.
--
-- see `remove_column`
--
-- @method remove_row
-- @tparam number|nil index Remove row at index. If `nil`, the last row is removed.
-- @treturn number The index of the removed row
function grid:remove_row(index)
    if index == nil or index > self._private.num_rows or index < 1 then
        index = self._private.num_rows
    end
    -- Update widget positions
    update_widgets_position(self._private.widgets, "vertical", index, "remove")
    -- Recalculate number of rows and columns
    update_dimension(self)
    return index
end


--- Add row border.
--
-- This method allows to set the width/color or a specific row rather than use
-- the same values for all the rows.
--
-- @DOC_wibox_layout_grid_add_row_border1_EXAMPLE@
--
-- @method add_row_border
-- @tparam integer index The row index. `1` is the top border (outer) border.
-- @tparam[opt=nil] integer|nil height The border height. If `nil` is passed,
--  then the `border_width.outer` will be user for index `1` and
-- `row_count + 1`,  otherwise, `border_width.inner` will be used.
-- @tparam[opt={}] table args
-- @tparam[opt=nil] color args.color The border color. If `nil` is passed,
--  then the `border_color.outer` will be user for index `1` and
-- `row_count + 1`,  otherwise, `border_color.inner` will be used.
-- @tparam[opt={1}] table args.dashes The dash pattern used for the line. By default,
--  it is a solid line.
-- @tparam[opt=0] number args.dash_offset If the line has `dashes`, then this is the
--  initial offset. Note that line are draw left to right and top to bottom.
-- @tparam[opt="butt"] string args.caps How the dashes ends are drawn. Either
--  `"butt"` (default), `"round"` or `"square"`
-- @noreturn
-- @see add_column_border

--- Add column border.
--
-- This method allows to set the width/color or a specific column rather than use
-- the same values for all the columns.
--
-- @DOC_wibox_layout_grid_add_column_border1_EXAMPLE@
--
-- @method add_column_border
-- @tparam integer index The column index. `1` is the top border (outer) border.
-- @tparam[opt=nil] integer|nil height The border height. If `nil` is passed,
--  then the `border_width.outer` will be user for index `1` and
-- `column_count + 1`,  otherwise, `border_width.inner` will be used.
-- @tparam[opt={}] table args
-- @tparam[opt=nil] color args.color The border color. If `nil` is passed,
--  then the `border_color.outer` will be user for index `1` and
-- `row_count + 1`,  otherwise, `border_color.inner` will be used.
-- @tparam[opt={1}] table args.dashes The dash pattern used for the line. By default,
--  it is a solid line.
-- @tparam[opt=0] number args.dash_offset If the line has `dashes`, then this is the
--  initial offset. Note that line are draw left to right and top to bottom.
-- @tparam[opt="butt"] string args.caps How the dashes ends are drawn. Either
--  `"butt"` (default), `"round"` or `"square"`
-- @noreturn
-- @see add_column_border

--- The border width.
--
-- @DOC_wibox_layout_grid_border_width1_EXAMPLE@
--
-- If `add_row_border` or `add_column_border` is used, it takes precedence and
-- is drawn on top of the `border_color` mask. Using both `border_width` and
-- `add_row_border` at the same time makes little sense:
--
-- @DOC_wibox_layout_grid_border_width2_EXAMPLE@
--
-- It is also possible to set the inner and outer borders separately:
--
-- @DOC_wibox_layout_grid_border_width4_EXAMPLE@
--
-- @property border_width
-- @tparam[opt=0] integer|table border_width
-- @tparam integer border_width.inner
-- @tparam integer border_width.outer
-- @propertytype integer Use the same value for inner and outer borders.
-- @propertytype table Specify a different value for the inner and outer borders.
-- @negativeallowed false
-- @see border_color
-- @see add_column_border
-- @see add_row_border

--- The border color for the table outer border.
-- @property border_color
-- @tparam[opt=0] color|table border_color
-- @tparam color border_color.inner
-- @tparam color border_color.outer
-- @propertytype color Use the same value for inner and outer borders.
-- @propertytype table Specify a different value for the inner and outer borders.
-- @see border_width

-- Return list of children
function grid:get_children()
    local ret = {}
    for _, data in ipairs(self._private.widgets) do
        table.insert(ret, data.widget)
    end
    return ret
end

-- Add list of children to the layout.
function grid:set_children(children)
    self:reset()
    if #children > 0 then
        self:add(unpack(children))
    end
end

-- Set the preferred orientation of the grid layout.
function grid:set_orientation(val)
    if self._private.orientation ~= val and (val == "horizontal" or val == "vertical") then
        self._private.orientation = val
    end
end

-- Set the minimum size for the columns.
function grid:set_min_cols_size(val)
    if self._private.min_cols_size ~= val and val >= 0 then
        self._private.min_cols_size = val
    end
end

-- Set the minimum size for the rows.
function grid:set_min_rows_size(val)
    if self._private.min_rows_size ~= val and val >= 0 then
        self._private.min_rows_size = val
    end
end

-- Force the number of columns of the layout.
function grid:set_forced_num_cols(val)
    if self._private.forced_num_cols ~= val then
        self._private.forced_num_cols = val
        update_dimension(self)
        self:emit_signal("widget::layout_changed")
    end
end

-- Force the number of rows of the layout.
function grid:set_forced_num_rows(val)
    if self._private.forced_num_rows ~= val then
        self._private.forced_num_rows = val
        update_dimension(self)
        self:emit_signal("widget::layout_changed")
    end
end

function grid:get_row_count()
    return self._private.num_rows
end

function grid:get_column_count()
    return self._private.num_cols
end

function grid:set_border_width(val)
    self._private.border_width = type(val) == "table" and val or {
        inner = val or 0,
        outer = val or 0,
    }

    -- Enforce integers. Not doing so makes the masking code more complex. Also,
    -- most of the time, not using integer is probably an user mistake (DPI
    -- related or ratio related).
    self._private.border_width.inner = gmath.round(self._private.border_width.inner)
    self._private.border_width.outer = gmath.round(self._private.border_width.outer)

    -- Drawing the border takes both a lot of memory (for the cached masks)
    -- and CPU, so make sure it is no-op for the 99% of cases where there is
    -- no border.
    self._private.has_border = self._private.border_width.inner ~= 0
        or self._private.border_width.outer ~= 0

    self:emit_signal("property::border_width", self._private.border_width)
    self:emit_signal("widget::layout_changed")
end

function grid:set_border_color(val)
    if type(val) == "table" then
          self._private.border_color = {
            inner = gcolor(val.inner),
            outer = gcolor(val.outer),
        }
    else
        self._private.border_color = {
            inner = gcolor(val),
            outer = gcolor(val),
        }
    end
    self:emit_signal("property::border_color", self._private.border_color)
    self:emit_signal("widget::redraw_needed")
end

function grid:add_row_border(index, height, args)
    self._private.has_border = true
    self._private.custom_border_width.rows[index] = {
        size   = height,
        color  = args.color and gcolor(args.color),
        dashes = args.dashes,
        offset = args.dash_offset,
        caps   = args.caps,
    }

    self:emit_signal("widget::layout_changed")
end

function grid:add_column_border(index, width, args)
    self._private.has_border = true
    self._private.custom_border_width.cols[index] = {
        size   = width,
        color  = args.color and gcolor(args.color),
        dashes = args.dashes,
        offset = args.dash_offset,
        caps   = args.caps,
    }

    self:emit_signal("widget::layout_changed")
end

-- Set the grid properties
for _, prop in ipairs(properties) do
    if not grid["set_" .. prop] then
        grid["set_"..prop] = function(self, value)
            if self._private[prop] ~= value then
                self._private[prop] = value
                self:emit_signal("widget::layout_changed")
            end
        end
    end
    if not grid["get_" .. prop] then
        grid["get_"..prop] = function(self)
            return self._private[prop]
        end
    end
end

-- Set the directional grid properties
-- create a couple of properties by prepending `horizontal_` and `vertical_`
-- create a common property for the two directions:
-- setting the common property sets both directional properties
-- getting the common property returns the directional property
-- defined by the `orientation` property
for _, prop in ipairs(dir_properties) do
    for _,dir in ipairs{"horizontal", "vertical"} do
        local dir_prop = dir .. "_" .. prop
        grid["set_"..dir_prop] = function(self, value)
            if self._private[dir_prop] ~= value then
                self._private[dir_prop] = value
                self:emit_signal("widget::layout_changed")
            end
        end
        grid["get_"..dir_prop] = function(self)
            return self._private[dir_prop]
        end
    end

    -- Non-directional options
    grid["set_"..prop] = function(self, value)
        if self._private["horizontal_"..prop] ~= value or self._private["vertical_"..prop] ~= value then
            self._private["horizontal_"..prop] = value
            self._private["vertical_"..prop] = value
            self:emit_signal("widget::layout_changed")
        end
    end
    grid["get_"..prop] = function(self)
        return self._private[self._private.orientation .. "_" .. prop]
    end
end


-- Return two tables of the fitted sizes of the rows and columns
-- @treturn table,table Tables of row heights and column widths
local function get_grid_sizes(self, context, orig_width, orig_height)
    local rows_size = {}
    local cols_size = {}

    -- Set the row and column sizes to the minimum value
    for i = 1,self._private.num_rows do
        rows_size[i] = self._private.min_rows_size
    end
    for j = 1,self._private.num_cols do
        cols_size[j] = self._private.min_cols_size
    end

    -- Calculate cell sizes
    for _, data in ipairs(self._private.widgets) do
        local w, h = base.fit_widget(self, context, data.widget, orig_width, orig_height)
        h = math.max( self._private.min_rows_size, h / data.row_span )
        w = math.max( self._private.min_cols_size, w / data.col_span )
        -- update the row and column maximum size
        for i = data.row, data.row + data.row_span - 1 do
            if h > rows_size[i] then rows_size[i] = h end
        end
        for j = data.col, data.col + data.col_span - 1 do
            if w > cols_size[j] then cols_size[j] = w end
        end
    end
    return rows_size, cols_size
end

-- All the code to get the width of a specific border.
--
-- This table module supports partial borders and "just add a border" modes.
local function setup_border_widths(self)
    self._private.border_width = {inner = 0, outer = 0}
    self._private.custom_border_width = {rows = {}, cols = {}}
    self._private.border_color = {}

    -- Use a metatable to get the defaults.
    local function meta_border_common(custom, row_or_col)
        return setmetatable({}, {
            __index = function(_, k)
                -- Handle custom borders.
                if custom[k] then
                    return custom[k].size
                end

                local size = self[row_or_col.."_count"]
                if k == 1 or k == size + 1 then
                    return self._private.border_width.outer
                else
                    return self._private.border_width.inner
                end
            end
        })
    end

    local hfb = self._private.custom_border_width.rows
    local vfb = self._private.custom_border_width.cols

    self._private.meta_borders = {
        rows = meta_border_common(hfb, "row"),
        cols = meta_border_common(vfb, "column"),
    }
end

-- Fit the grid layout into the given space.
-- @param context The context in which we are fit.
-- @param orig_width The available width.
-- @param orig_height The available height.
function grid:fit(context, orig_width, orig_height)
    local width, height = orig_width, orig_height

    -- Calculate the space needed
    local function fit_direction(dir, sizes, border_widths)
        local m = border_widths[1]
        local space = self._private[dir .. "_spacing"]

        -- First border
        m = m > 0 and m + space or m

        if self._private[dir .. "_homogeneous"] then
            local max = max_value(sizes)

            -- all the columns/rows have the same size
            if self._private.has_border then

                -- Not all borders are identical, so the loop is required.
                for i in ipairs(sizes) do
                    local bw = border_widths[i+1]

                    -- When there is a border, it needs the spacing on both sides.

                    m = m + max + (space*(bw > 0 and 2 or 1)) + bw
                end
            else
                -- Much simpler.
                m = #sizes * max + (#sizes - 1) * space
            end
        else
            -- sum the columns/rows size
            for i, s in ipairs(sizes) do
                local bw = border_widths[i+1]

                -- When there is a border, it needs the spacing on both sides.
                m = m + s + (space * (bw > 0 and 2 or 1)) + bw
            end

        end

        return m
    end

    -- fit matrix cells
    local rows_size, cols_size = get_grid_sizes(self, context, width, height)

    -- compute the width
    local borders = self._private.meta_borders
    local used_width_max  = fit_direction("horizontal", cols_size, borders.cols)
    local used_height_max = fit_direction("vertical", rows_size, borders.rows)

    return used_width_max, used_height_max
end

local function layout_common(self, context, width, height, h_homogeneous, v_homogeneous)
    local result, areas = {}, {}
    local hspacing, vspacing = self._private.horizontal_spacing, self._private.vertical_spacing

    -- Fit matrix cells
    local rows_size, cols_size = get_grid_sizes(self, context, width, height)
    local total_expected_width, total_expected_height = sum_values(cols_size), sum_values(rows_size)

    local h_bw, v_bw = self._private.meta_borders.cols, self._private.meta_borders.rows

    -- Do it once, the result wont change unless widgets are added.
    if self._private.has_border and not self._private.area_cache.total_horizontal_border_width then
        -- Also add the "second" spacing here. This avoid having some `if` below.
        local total_h = h_bw[1] + h_bw[#cols_size+1] + 1*hspacing
        local total_v = v_bw[1] + v_bw[#rows_size+1] + 1*vspacing

        for j = 1, #cols_size do
            local bw = h_bw[j+1]
            total_h =  total_h + bw + hspacing*(bw > 0 and 1 or 0)
        end

        for i = 1, #rows_size do
            local bw = v_bw[i+1]
            total_v = total_v + bw + vspacing*(bw > 0 and 1 or 0)
        end

        self._private.area_cache.total_horizontal_border_width = total_h - h_bw[1]
        self._private.area_cache.total_vertical_border_width = total_v - v_bw[1]
    end

    local total_h = self._private.area_cache.total_horizontal_border_width or 0
    local total_v = self._private.area_cache.total_vertical_border_width or 0

    -- Figure out the maximum size we can give out to sub-widgets
    local single_width, single_height = max_value(cols_size), max_value(rows_size)

    if self._private.horizontal_expand then
        single_width = (width - (self._private.num_cols-1)*hspacing - total_h) / self._private.num_cols
    end

    if self._private.vertical_expand then
        single_height = (height - (self._private.num_rows-1)*vspacing - total_v) / self._private.num_rows
    end

    -- Calculate the position and size to place the widgets
    local cumul_width, cumul_height = {}, {}
    local c_hor, c_ver = h_bw[1], v_bw[1]

    -- If there is an outer border, then it needs inner spacing too.
    c_hor, c_ver = c_hor > 0 and c_hor + hspacing or 0, c_ver > 0 and c_ver + vspacing or 0

    for j = 1, #cols_size do
        cumul_width[j] = c_hor

        if h_homogeneous then
            cols_size[j] = math.max(self._private.min_cols_size, single_width)
        elseif self._private.horizontal_expand then
            local hpercent = self._private.num_cols * single_width * cols_size[j] / total_expected_width
            cols_size[j] = math.max(self._private.min_cols_size, hpercent)
        end

        local bw = h_bw[j+1]
        c_hor = c_hor + cols_size[j] + (bw > 0 and 2 or 1)*hspacing + bw
    end

    cumul_width[#cols_size + 1] = c_hor

    for i = 1, #rows_size do
        cumul_height[i] = c_ver

        if v_homogeneous then
            rows_size[i] = math.max(self._private.min_rows_size, single_height)
        elseif self._private.vertical_expand then
            local vpercent = self._private.num_rows * single_height * rows_size[i] / total_expected_height
            rows_size[i] = math.max(self._private.min_rows_size, vpercent)
        end

        local bw = v_bw[i+1]
        c_ver = c_ver + rows_size[i] + (bw > 0 and 2 or 1)*vspacing + bw
    end

    cumul_height[#rows_size + 1] = c_ver

    -- Place widgets
    local fill_space = true  -- should be fill_space property?
    for _, v in pairs(self._private.widgets) do
        local x, y, w, h

        -- If there is a border, then the spacing is needed on both sides.
        local col_bw, row_bw = h_bw[v.col+v.col_span], v_bw[v.row+v.row_span]
        local col_spacing = hspacing * (col_bw > 0 and 2 or 1)
        local row_spacing = vspacing * (row_bw > 0 and 2 or 1)

        -- Round numbers to avoid decimals error, force to place tight widgets
        -- and avoid redraw glitches
        x = math.floor(cumul_width[v.col])
        y = math.floor(cumul_height[v.row])
        w = math.floor(cumul_width[v.col + v.col_span] - col_spacing - x - col_bw)
        h = math.floor(cumul_height[v.row + v.row_span] - row_spacing - y - row_bw)

        -- Handle large spacing and/or border_width. The grid doesn't support
        -- dropping widgets. It would be very hard to implement.
        w, h = math.max(0, w), math.max(0, h)

        -- Recalculate the width so the last widget fits
        if (fill_space or self._private.horizontal_expand) and x + w > width then
            w = math.floor(math.max(self._private.min_cols_size, width - x))
        end
        -- Recalculate the height so the last widget fits
        if (fill_space or self._private.vertical_expand) and y + h > height then
            h = math.floor(math.max(self._private.min_rows_size, height - y))
        end
        -- Place the widget if it fits in the area
        if x + w <= width and y + h <= height then
            table.insert(result, base.place_widget_at(v.widget, x, y, w, h))
            table.insert(areas, {
                x      = x - hspacing,
                y      = y - vspacing,
                width  = w + col_spacing,
                height = h + row_spacing,
            })
        end
    end

    -- Sometime, the `:fit()` size and `:layout()` size are different, thus it's
    -- important to say where the widget actually ends.
    areas.end_x = cumul_width[#cumul_width] - hspacing
    areas.end_y = cumul_height[#cumul_height] - vspacing
    areas.column_count = #cols_size
    areas.row_count = #rows_size
    areas.cols = cumul_width
    areas.rows = cumul_height

    return result, areas
end

local function get_area_cache_hash(width, height)
   return width*1.5+height*15
end

-- Layout a grid layout.
-- @param context The context in which we are drawn.
-- @param width The available width.
-- @param height The available height.
function grid:layout(context, width, height)
    local l, areas = layout_common(
        self,
        context,
        width,
        height,
        self._private.horizontal_homogeneous,
        self._private.vertical_homogeneous
    )

    self._private.area_cache[get_area_cache_hash(width, height)] = areas

    return l
end

local function create_border_mask(self, areas, default_color)
    if areas.surface then return areas.surface end

    local meta = self._private.meta_borders

    local top, bottom = meta.rows[1], meta.rows[areas.row_count+1]
    local left, right = meta.cols[1], meta.cols[areas.column_count+1]

    -- A1 is fine because :layout() aligns to pixel boundary and `border_width`
    -- are integers.
    local img = cairo.RecordingSurface(cairo.Content.COLOR_ALPHA, cairo.Rectangle {
        x      = 0,
        y      = 0,
        width  = areas.end_x + right,
        height = areas.end_y + bottom
    })
    local cr = cairo.Context(img)
    cr:set_source(default_color)

    local bw_i, bw_o = self._private.border_width.inner, self._private.border_width.outer

    if bw_i ~= bw_o then
        if bw_o then
            if self._private.border_color.outer then
                cr:set_source(self._private.border_color.outer)
            end

            -- Clip the outside region. It cannot use `cr:set_line_width()` because
            -- each border might be different.
            cr:rectangle(0, 0, areas.end_x, top)
            cr:rectangle(0, areas.end_y - bottom, areas.end_x, bottom)
            cr:rectangle(0, top, left, areas.end_y - top - bottom)
            cr:rectangle(areas.end_x - right, top, right, areas.end_y - top - bottom)
            cr:clip()
            cr:paint()
            cr:reset_clip()
        end

        cr:rectangle(left, top, areas.end_x - top - bottom, areas.end_y - left - right)
        cr:clip()
    else
        cr:rectangle(0,0, areas.end_x, areas.end_y)
        cr:clip()
    end

    if bw_i then
        if self._private.border_color.inner then
            cr:set_source(self._private.border_color.inner)
        end
        cr:rectangle(0, 0, areas.end_x, areas.end_y)
        cr:fill()
    end

    -- Add the custom horizontal and borders.
    -- This is a lifeline for users who want borders only on specific places.
    -- Implementing word processing style borders would be overkill and
    -- too hard to maintain.
    for _, orientation in ipairs { "rows", "cols" } do
        for row, args in pairs(self._private.custom_border_width[orientation]) do
            local line_height = meta[orientation][row]
            cr:save()
            cr:rectangle(0,0, areas.end_x, areas.end_y)
            cr:clip()
            cr:set_line_width(line_height)

            if args.dashes then
                cr:set_dash(args.dashes, #args.dashes, args.offset or 0)
            end

            if args.caps then
                cr:set_line_cap(cairo.LineCap[args.caps:upper()])
            end

            cr:set_source(args.color)

            -- Cairo draw the stroke equally on both side, for `line_height/2` is
            -- needed.
            local y = (row == 1 and line_height or areas[orientation][row] or 0) - math.ceil(line_height/2)

            if orientation == "rows" then
                cr:move_to(0, y)
                cr:line_to(areas.end_x, y)
            else
                cr:move_to(y, 0)
                cr:line_to(y, areas.end_y)
            end

            cr:stroke()
            cr:restore()
        end
    end

    -- Remove the area used by widgets. This needs to be done regardless of the
    -- border mode to handle row/col span.
    cr:set_operator(cairo.Operator.CLEAR)

    for _, area in ipairs(areas) do
        cr:rectangle(area.x, area.y, area.width, area.height)
    end

    cr:fill()

    areas.surface = img

    return img
end

-- Draw the border.
function grid:after_draw_children(ctx, cr, width, height)
    if not self._private.has_border then return end

    local hash = get_area_cache_hash(width, height)

    if not self._private.area_cache[hash] then
        self._private.area_cache[hash] = select(2, layout_common(
            self,
            ctx,
            width,
            height,
            self._private.horizontal_homogeneous,
            self._private.vertical_homogeneous
        ))
    end

    local areas = self._private.area_cache[hash]

    cr:set_source_surface(create_border_mask(self, areas, cr:get_source()), 0 ,0)
    cr:paint()
end

--- Reset the grid layout.
-- Remove all widgets and reset row and column counts
--
-- @method reset
-- @emits reset
-- @noreturn
function grid:reset()
    self._private.widgets = {}
    -- reset the number of columns and rows to the forced value or to 0
    self._private.num_rows = self._private.forced_num_rows ~= nil
        and self._private.forced_num_rows or 0
    self._private.num_cols = self._private.forced_num_cols ~= nil
        and self._private.forced_num_cols or 0
    -- emit signals
    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::reset")
end

--- When the layout is reset.
-- This signal is emitted when the layout has been reset,
-- all the widgets removed and the row and column counts reset.
-- @signal widget::reset



--- Return a new grid layout.
--
-- A grid layout sets widgets in a grids of custom number of rows and columns.
-- @tparam[opt="y"] string orientation The preferred grid extension direction.
-- @constructorfct wibox.layout.grid
local function new(orientation)
    -- Preference for vertical direction: fill rows first, extend grid with new row
    local dir = (orientation == "horizontal"or orientation == "vertical")
        and orientation or "vertical"

    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, grid, true)

    ret._private.orientation = dir
    ret._private.widgets = {}
    ret._private.num_rows = 0
    ret._private.num_cols = 0
    ret._private.rows_size = {}
    ret._private.cols_size = {}

    ret._private.superpose       = false
    ret._private.forced_num_rows = nil
    ret._private.forced_num_cols = nil
    ret._private.min_rows_size   = 0
    ret._private.min_cols_size   = 0

    ret._private.horizontal_homogeneous = true
    ret._private.vertical_homogeneous   = true
    ret._private.horizontal_expand      = false
    ret._private.vertical_expand        = false
    ret._private.horizontal_spacing     = 0
    ret._private.vertical_spacing       = 0

    ret._private.area_cache, ret._private.border_color = {}, {}

    ret:connect_signal("widget::layout_changed", function(self)
        self._private.area_cache = {}
    end)

    setup_border_widths(ret)

    return ret
end

--- Return a new horizontal grid layout.
--
-- Each new widget is positioned below the last widget on the same column
-- up to `forced_num_rows`. Then the next column is filled, creating it if it doesn't exist.
-- @tparam number|nil forced_num_rows Forced number of rows (`nil` for automatic).
-- @tparam widget ... Widgets that should be added to the layout.
-- @constructorfct wibox.layout.grid.horizontal
function grid.horizontal(forced_num_rows, widget, ...)
    local ret = new("horizontal")
    ret:set_forced_num_rows(forced_num_rows)

    if widget then
        ret:add(widget, ...)
    end

    return ret
end

--- Return a new vertical grid layout.
--
-- Each new widget is positioned left of the last widget on the same row
-- up to `forced_num_cols`. Then the next row is filled, creating it if it doesn't exist.
-- @tparam number|nil forced_num_cols Forced number of columns (`nil` for automatic).
-- @tparam widget ... Widgets that should be added to the layout.
-- @constructorfct wibox.layout.grid.vertical
function grid.vertical(forced_num_cols, widget, ...)
    local ret = new("vertical")
    ret:set_forced_num_cols(forced_num_cols)

    if widget then
        ret:add(widget, ...)
    end

    return ret
end


function grid.mt:__call(...)
    return new(...)
end

--@DOC_fixed_COMMON@

return setmetatable(grid, grid.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
