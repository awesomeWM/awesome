--- awesome key API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("key")

--- Key object.
-- @field key The key to press to triggers an event.
-- @field keysym Same as key, but return the name of the key symbol. It can
-- be identical to key, but for characters like '.' it will return 'period'.
-- @field modifiers The modifier key that should be pressed while the key is
-- pressed. An array with all the modifiers. Valid modifiers are: Any, Mod1,
-- Mod2, Mod3, Mod4, Mod5, Shift, Lock and Control.
-- @class table
-- @name key

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
