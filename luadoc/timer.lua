--- awesome timer API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
module("timer")

--- Timer object. This type of object is useful when triggering events in a repeatedly.
-- The timer will emit the "timeout" signal every N seconds, N being the timeout value.
-- @field timeout Interval in seconds to emit the timeout signal.
-- Can be any value, including floating ones (i.e. 1.5 second).
-- @field started Read-only boolean field indicating if the timer has been started.
-- @class table
-- @name timer

--- Start the timer.
-- @param -
-- @name start
-- @class function

--- Stop the timer.
-- @param -
-- @name stop
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
-- @return The number of timer objects alive.
-- @name instances
-- @class function
