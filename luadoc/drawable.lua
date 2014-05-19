--- awesome drawable API
-- @author Uli Schlachter &lt;psychon@znc.in&gt;
-- @copyright 2012 Uli Schlachter
-- @classmod drawable

--- Drawable object.
--
-- @field surface The drawable's cairo surface.
-- @function drawable

--- Get drawable geometry. The geometry consists of x, y, width and height.
--
-- @return A table with drawable coordinates and geometry.
-- @function geometry

--- Refresh the drawable. When you are drawing to the surface, you have
-- call this function when you are done to make the result visible.
--
-- @function refresh

--- Add a signal.
--
-- @param name A signal name.
-- @param func A function to call when the signal is emitted.
-- @function connect_signal

--- Remove a signal.
--
-- @param name A signal name.
-- @param func A function to remove.
-- @function disconnect_signal

--- Emit a signal.
--
-- @param name A signal name.
-- @param ... Various arguments, optional.
-- @function emit_signal

--- Get the number of instances.
--
-- @return The number of drawable objects alive.
-- @function instances
