--- awesome drawin API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("drawin")

--- Drawin object.
-- @field border_width Border width.
-- @field border_color Border color.
-- @field ontop On top of other windows.
-- @field cursor The mouse cursor.
-- @field visible Visibility.
-- @field opacity The opacity of the drawin, between 0 and 1.
-- @field type The window type (desktop, normal, dock, …).
-- @field x The x coordinates.
-- @field y The y coordinates.
-- @field width The width of the drawin.
-- @field height The height of the drawin.
-- @field drawable The drawin's drawable.
-- @field window The X window id.
-- @field shape_bounding The drawin's bounding shape as a (native) cairo surface.
-- @field shape_clip The drawin's clip shape as a (native) cairo surface.
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

--- Change a xproperty.
-- @param name The name of the X11 property
-- @param value The new value for the property
-- @name set_xproperty
-- @class function

--- Get the value of a xproperty.
-- @param name The name of the X11 property
-- @name get_xproperty
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
-- @return The number of drawin objects alive.
-- @name instances
-- @class function
