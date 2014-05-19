--- awesome D-Bus API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @module dbus

--- Register a D-Bus name to receive message from.
--
-- @param bus A string indicating if we are using system or session bus.
-- @param name A string with the name of the D-Bus name to register.
-- @return True if everything worked fine, false otherwise.
-- @function request_name

--- Release a D-Bus name.
--
-- @param bus A string indicating if we are using system or session bus.
-- @param name A string with the name of the D-Bus name to unregister.
-- @return True if everything worked fine, false otherwise.
-- @function release_name

--- Add a match rule to match messages going through the message bus.
--
-- @param bus A string indicating if we are using system or session bus.
-- @param name A string with the name of the match rule.
-- @function add_match

--- Remove a previously added match rule "by value"
-- (the most recently-added identical rule gets removed).
--
-- @param bus A string indicating if we are using system or session bus.
-- @param name A string with the name of the match rule.
-- @function remove_match

--- Add a signal receiver on the D-Bus.
--
-- @param interface A string with the interface name.
-- @param func The function to call.
-- @function connect_signal

--- Remove a signal receiver on the D-Bus.
--
-- @param interface A string with the interface name.
-- @param func The function to call.
-- @function disconnect_signal
