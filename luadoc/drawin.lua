--- awesome drawin API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("drawin")

--- Drawin object.
-- @field screen Screen number.
-- @field border_width Border width.
-- @field border_color Border color.
-- @field ontop On top of other windows.
-- @field cursor The mouse cursor.
-- @field visible Visibility.
-- @field opacity The opacity of the drawin, between 0 and 1.
-- @field x The x coordinates.
-- @field y The y coordinates.
-- @field width The width of the drawin.
-- @field height The height of the drawin.
-- @field surface A cairo surface as light user datum that can be used for drawing.
-- @class table
-- @name drawin

--- Get or set mouse buttons bindings to a drawin.
-- @param buttons_table A table of buttons objects, or nothing.
-- @name buttons
-- @class function

--- Get or set drawin struts.
-- @param strut A table with new strut, or nothing
-- @return The drawin strut in a table.
-- @name struts
-- @class function

--- Get or set drawin geometry. That's the same as accessing or setting the x, y, width or height
-- properties of a drawin.
-- @param A table with coordinates to modify.
-- @return A table with drawin coordinates and geometry.
-- @name geometry
-- @class function

--- Refresh the drawin. When you are drawing to the window's surface, you have
-- call this function when you are done to make the result visible.
-- @param no_params luadoc is buggy.
-- @name refresh
-- @class function

--- Add a signal.
-- @param name A signal name.
-- @param func A function to call when the signal is emitted.
-- @name connect_signal
-- @class function

--- Remove a signal.
-- @param name A signal name.
-- @param func A function to remove.
-- @name disconnect_signal
-- @class function

--- Emit a signal.
-- @param name A signal name.
-- @param ... Various arguments, optional.
-- @name emit_signal
-- @class function

--- Get the number of instances.
-- @param no_params luadoc is buggy.
-- @return The number of drawin objects alive.
-- @name instances
-- @class function
