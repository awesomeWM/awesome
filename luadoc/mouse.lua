--- awesome mouse API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("mouse")

--- Mouse library.
-- @field screen Mouse screen number.
-- @class table
-- @name mouse

--- Get or set the mouse coords.
-- @param coords_table None or a table with x and y keys as mouse coordinates.
-- @param silent Disable mouse::enter or mouse::leave events that could be triggered by the pointer when moving.
-- @return A table with mouse coordinates.
-- @name coords
-- @class function

--- Get the client or any object which is under the pointer.
-- @return A client or nil.
-- @name object_under_pointer
-- @class function
