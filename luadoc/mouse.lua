--- awesome mouse API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("mouse")

--- Mouse library.
-- @field screen Mouse screen number.
-- @table mouse

--- A table with X and Y coordinates.
-- @field x X coordinate.
-- @field y Y coordinate.
-- @table coords_table

--- Get or set the mouse coords.
-- @tparam coords_table coords_table None or a table with x and y keys as mouse coordinates.
-- @tparam boolean silent Disable mouse::enter or mouse::leave events that could be triggered by the pointer when moving.
-- @treturn coords_table A table with mouse coordinates.
-- @function coords

--- Get the client or any object which is under the pointer.
-- @treturn client.client|nil A client or nil.
-- @function object_under_pointer
