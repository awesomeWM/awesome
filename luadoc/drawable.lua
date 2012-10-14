--- awesome drawable API
-- @author Uli Schlachter &lt;psychon@znc.in&gt;
-- @copyright 2012 Uli Schlachter
module("drawable")

--- Drawable object.
-- @field surface The drawable's cairo surface.
-- @name drawable

--- Get drawable geometry. The geometry consists of x, y, width and height.
-- @param no_params luadoc is buggy.
-- @return A table with drawable coordinates and geometry.
-- @name geometry
-- @class function

--- Refresh the drawable. When you are drawing to the surface, you have
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
-- @return The number of drawable objects alive.
-- @name instances
-- @class function
