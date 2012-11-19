--- awesome keygrabber API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("keygrabber")

---
-- Grab keyboard and read pressed keys, calling callback function at each key
-- press, until keygrabber.stop is called.
-- The callback function is passed three arguments:
-- a table containing modifiers keys, a string with the key pressed and a
-- string with either "press" or "release" to indicate the event type.
-- @param callback A callback function as described above.
-- @name run
-- @class function
-- @usage Following function can be bound to a key, and used to resize a client
-- using keyboard.
-- <p><code>
-- function resize(c) <br/>
--   keygrabber.run(function(mod, key, event) </br>
--     if event == "release" then return end </br></br>
--
--     if     key == 'Up'   then awful.client.moveresize(0, 0, 0, 5, c) <br/>
--     elseif key == 'Down' then awful.client.moveresize(0, 0, 0, -5, c) <br/>
--     elseif key == 'Right' then awful.client.moveresize(0, 0, 5, 0, c) <br/>
--     elseif key == 'Left'  then awful.client.moveresize(0, 0, -5, 0, c) <br/>
--     else   keygrabber.stop() <br/>
--     end <br/><br/>
--
--   end) <br/>
-- end <br/>
-- </code></p>

--- Stop grabbing the keyboard.
-- @name stop
-- @class function

--- Check if the keygrabber is running.
-- @return A boolean value, true if running, false otherwise.
-- @name isrunning
-- @class function
