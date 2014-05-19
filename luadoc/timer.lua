--- awesome timer API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @classmod timer

--- Timer object. This type of object is useful when triggering events in a repeatedly.
-- The timer will emit the "timeout" signal every N seconds, N being the timeout value.
--
-- @field timeout Interval in seconds to emit the timeout signal.
--   Can be any value, including floating ones (i.e. 1.5 second).
-- @field started Read-only boolean field indicating if the timer has been started.
-- @table timer

--- Start the timer.
--
-- @function start

--- Stop the timer.
--
-- @function stop

--- Restart the timer.
--
-- @function again

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
-- @return The number of timer objects alive.
-- @function instances
