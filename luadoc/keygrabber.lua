--- awesome keygrabber API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("keygrabber")

--- Grab keyboard and read pressed keys, calling callback function at each key
-- pressed. The callback function must return a boolean value: true to
-- continue grabbing, false to stop.
-- The function is called with 3 arguments:
-- a table containing modifiers keys, a string with the key pressed and a
-- string with eithe "press" or "release" to indicate the event type.
-- @param func A callback function as described above.
-- @name run
-- @class function

--- Stop grabbing the keyboard.
-- @param -
-- @name stop
-- @class function
