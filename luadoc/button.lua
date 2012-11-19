--- awesome button API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("button")

--- Button object.
-- @field button The mouse button number, or 0 for any button.
-- @field modifiers The modifier key table that should be pressed while the
-- button is pressed.
-- @class table
-- @name button

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
-- @return The number of button objects alive.
-- @name instances
-- @class function
