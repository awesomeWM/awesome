---------------------------------------------------------------------------
--- Place multiple widgets in multiple rows and columns.
--
-- Widgets spanning several columns or rows cannot be included using the
-- declarative system.
-- Instead, create the grid layout and call the `add_widget_at` method.
--
--@DOC_wibox_layout_grid_imperative_EXAMPLE@
--
-- Using the declarative system, widgets are automatically added next to each
-- other spanning only one cell.
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
local base = require("wibox.widget.base")

local grid = { mt = {} }

local properties = {
                    "orientation", "superpose",
                    "forced_num_rows", "forced_num_cols",
                    "min_cols_size", "min_rows_size",
                   }

local dir_properties = { "spacing", "homogeneous", "expand" }



--- Set the preferred orientation of the grid layout.
--
-- Allowed values are "horizontal" and "vertical".
-- When calling `get_next_empty`, empty cells are browsed differently:
--
-- * for "horizontal", the grid can be extended horizontally. The current
--  column is filled first; if no empty cell is found up to `forced_num_rows`,
--  the next column is filled, creating it if it does not exist.
--
-- * for "vertical", the grid can be extended vertically. The current row is
--  filled first; if no empty cell is found up to `forced_num_cols`, the next
--  row is filled, creating it if it does not exist.
--
--@DOC_wibox_layout_grid_orientation_EXAMPLE@
-- @tparam[opt="vertical"] string orientation Preferred orientation: "horizontal" or "vertical".
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
-- @tparam[opt=nil] number|nil forced_num_rows Forced number of rows (`nil` for automatic).

--- Force the number of columns of the layout.
-- @property forced_num_cols
-- @tparam[opt=nil] number|nil forced_num_cols Forced number of columns (`nil` for automatic).

--- Set the minimum size for the columns.
--
--@DOC_wibox_layout_grid_min_size_EXAMPLE@
-- @tparam[opt=0] number min_cols_size Minimum size of the columns.
-- @property min_cols_size

--- Set the minimum size for the rows.
-- @tparam[opt=0] number min_rows_size Minimum size of the rows.
-- @property min_rows_size

--- The spacing between columns.
--
-- see `spacing`
--
-- @tparam[opt=0] number horizontal_spacing The spacing
-- @property horizontal_spacing

--- The spacing between rows.
--
-- see `spacing`
--
-- @tparam[opt=0] number vertical_spacing The spacing
-- @property vertical_spacing

--- The spacing between rows and columns.
-- Set both `horizontal_spacing` and `vertical_spacing` to the same value.
-- Get the value `horizontal_spacing` or `vertical_spacing` defined by the
-- preferred `orientation`.
--
--@DOC_wibox_layout_grid_spacing_EXAMPLE@
-- @tparam[opt=0] number spacing The spacing
-- @property spacing

--- Controls if the columns are expanded to use all the available width.
--
-- see `expand`
--
-- @tparam[opt=false] boolean horizontal_expand Expand the grid into the available space
-- @property horizontal_expand

--- Controls if the rows are expanded to use all the available height.
--
-- see `expand`
--
-- @tparam[opt=false] boolean vertical_expand Expand the grid into the available space
-- @property vertical_expand

--- Controls if the columns/rows are expanded to use all the available space.
-- Set both `horizontal_expand` and `vertical_expand` to the same value.
-- Get the value `horizontal_expand` or `vertical_expand` defined by the
-- preferred `orientation`.
--
--@DOC_wibox_layout_grid_expand_EXAMPLE@
-- @tparam[opt=false] boolean expand Expand the grid into the available space
-- @property expand

--- Controls if the columns all have the same width or if the width of each
-- column depends on the content.
--
-- see `homogeneous`
--
-- @tparam[opt=true] boolean horizontal_homogeneous All the columns have the same width.
-- @property horizontal_homogeneous

--- Controls if the rows all have the same height or if the height of each row
-- depends on the content.
--
-- see `homogeneous`
--
-- @tparam[opt=true] boolean vertical_homogeneous All the rows have the same height.
-- @property vertical_homogeneous

--- Controls if the columns/rows all have the same size or if the size depends
-- on the content.
-- Set both `horizontal_homogeneous` and `vertical_homogeneous` to the same value.
-- Get the value `horizontal_homogeneous` or `vertical_homogeneous` defined
-- by the preferred `orientation`.
--
--@DOC_wibox_layout_grid_expand_EXAMPLE@
-- @tparam[opt=true] boolean homogeneous All the columns/rows have the same size.
-- @property homogeneous

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
-- @method add
-- @param ... Widgets that should be added (must at least be one)
function grid:add(...)
    local args = { n=select('#', ...), ... }
    assert(args.n > 0, "need at least one widget to add")
    local row, column
    for i=1, args.n do
        -- Get the next empty coordinate to insert the widget
        row, column = self:get_next_empty(row, column)
        self:add_widget_at(args[i], row, column, 1, 1)
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


-- Fit the grid layout into the given space.
-- @param context The context in which we are fit.
-- @param orig_width The available width.
-- @param orig_height The available height.
function grid:fit(context, orig_width, orig_height)
    local width, height = orig_width, orig_height

    -- Calculate the space needed
    local function fit_direction(dir, sizes)
        local m = 0
        if self._private[dir .. "_homogeneous"] then
            -- all the columns/rows have the same size
            m = #sizes * max_value(sizes) + (#sizes - 1) * self._private[dir .. "_spacing"]
        else
            -- sum the columns/rows size
            for _,s in ipairs(sizes) do
                m = m + s + self._private[dir .. "_spacing"]
            end
            m = m - self._private[dir .. "_spacing"]
        end
        return m
    end

    -- fit matrix cells
    local rows_size, cols_size = get_grid_sizes(self, context, width, height)

    -- compute the width
    local used_width_max  = fit_direction("horizontal", cols_size)
    local used_height_max = fit_direction("vertical", rows_size)

    return used_width_max, used_height_max
end

-- Layout a grid layout.
-- @param context The context in which we are drawn.
-- @param width The available width.
-- @param height The available height.
function grid:layout(context, width, height)
    local result = {}
    local hspacing, vspacing = self._private.horizontal_spacing, self._private.vertical_spacing

    -- Fit matrix cells
    local rows_size, cols_size = get_grid_sizes(self, context, width, height)
    local total_expected_width, total_expected_height = sum_values(cols_size), sum_values(rows_size)

    -- Figure out the maximum size we can give out to sub-widgets
    local single_width, single_height = max_value(cols_size), max_value(rows_size)
    if self._private.horizontal_expand then
        single_width  = (width - (self._private.num_cols-1)*hspacing) / self._private.num_cols
    end
    if self._private.vertical_expand then
        single_height = (height - (self._private.num_rows-1)*vspacing) / self._private.num_rows
    end

    -- Calculate the position and size to place the widgets
    local cumul_width, cumul_height = {}, {}
    local cw, ch = 0, 0
    for j = 1, #cols_size do
        cumul_width[j] = cw
        if self._private.horizontal_homogeneous then
            cols_size[j] = math.max(self._private.min_cols_size, single_width)
        elseif self._private.horizontal_expand then
            local hpercent = self._private.num_cols * single_width * cols_size[j] / total_expected_width
            cols_size[j] = math.max(self._private.min_cols_size, hpercent)
        end
        cw = cw + cols_size[j] + hspacing
    end
    cumul_width[#cols_size + 1] = cw
    for i = 1, #rows_size do
        cumul_height[i] = ch
        if self._private.vertical_homogeneous then
            rows_size[i] = math.max(self._private.min_rows_size, single_height)
        elseif self._private.vertical_expand then
            local vpercent = self._private.num_rows * single_height * rows_size[i] / total_expected_height
            rows_size[i] = math.max(self._private.min_rows_size, vpercent)
        end
        ch = ch + rows_size[i] + vspacing
    end
    cumul_height[#rows_size + 1] = ch

    -- Place widgets
    local fill_space = true  -- should be fill_space property?
    for _, v in pairs(self._private.widgets) do
        local x, y, w, h
        -- Round numbers to avoid decimals error, force to place tight widgets
        -- and avoid redraw glitches
        x = math.floor(cumul_width[v.col])
        y = math.floor(cumul_height[v.row])
        w = math.floor(cumul_width[v.col + v.col_span] - hspacing - x)
        h = math.floor(cumul_height[v.row + v.row_span] - vspacing - y)
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
        end
    end
    return result
end


--- Reset the grid layout.
-- Remove all widgets and reset row and column counts
--
-- **Signal:** widget::reset
-- @method reset
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
