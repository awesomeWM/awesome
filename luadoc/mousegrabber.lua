--- awesome mousegrabber API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("mousegrabber")

--- Grab the mouse pointer and list motions, calling callback function at each
-- motion. The callback function must return a boolean value: true to
-- continue grabbing, false to stop.
-- The function is called with one argument:
-- a table containing modifiers pointer coordinates.
-- @param func A callback function as described above.
-- @param cursor The name of a X cursor to use while grabbing.
-- @name run
-- @class function

--- Stop grabbing the mouse pointer.
-- @param -
-- @name stop
-- @class function
