--- awesome D-Bus API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("dbus")

--- Register a D-Bus name to receive message from.
-- @param bus A string indicating if we are using system or session bus.
-- @param name A string with the name of the D-Bus name to register.
-- @return True if everything worked fine, false otherwise.
-- @name request_name
-- @class function

--- Release a D-Bus name.
-- @param bus A string indicating if we are using system or session bus.
-- @param name A string with the name of the D-Bus name to unregister.
-- @return True if everything worked fine, false otherwise.
-- @name release_name
-- @class function

--- Add a match rule to match messages going through the message bus.
-- @param bus A string indicating if we are using system or session bus.
-- @param name A string with the name of the match rule.
-- @name add_match
-- @class function

--- Remove a previously added match rule "by value"
-- (the most recently-added identical rule gets removed).
-- @param bus A string indicating if we are using system or session bus.
-- @param name A string with the name of the match rule.
-- @name remove_match
-- @class function

--- Add a signal receiver on the D-Bus.
-- @param interface A string with the interface name.
-- @param func The function to call.
-- @name connect_signal
-- @class function

--- Remove a signal receiver on the D-Bus.
-- @param interface A string with the interface name.
-- @param func The function to call.
-- @name disconnect_signal
-- @class function

-- Emit a signal on the D-Bus.
-- @param bus A string indicating if we are using system or session bus.
-- @param path A string with the dbus path.
-- @param interface A string with the dbus interface.
-- @param method A string with the dbus method name.
-- @param type_1st_arg type of 1st argument
-- @param value_1st_arg value of 1st argument
-- @param type_2nd_arg type of 2nd argument
-- @param value_2nd_arg value of 2nd argument
-- ... etc
-- @name emit_signal
-- @class function
--
