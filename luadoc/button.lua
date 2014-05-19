--- awesome button API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @classmod button

--- Button object.
--
-- @tfield int button The mouse button number, or 0 for any button.
-- @tfield table modifiers The modifier key table that should be pressed while the
--   button is pressed.
-- @table button

--- Add a signal.
-- @tparam string name A signal name.
-- @tparam func func A function to call when the signal is emitted.
-- @function connect_signal

--- Remove a signal.
-- @tparam string name A signal name.
-- @tparam func func A function to remove.
-- @function disconnect_signal

--- Emit a signal.
-- @tparam string name A signal name.
-- @param ... Various arguments, optional.
-- @function emit_signal

--- Get the number of instances.
-- @treturn int The number of button objects alive.
-- @function instances
