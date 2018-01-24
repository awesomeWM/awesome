---------------------------------------------------------------------------
--- Keygrabber Stack
--
-- @author dodo
-- @copyright 2012 dodo
-- @module awful.keygrabber
---------------------------------------------------------------------------

local ipairs = ipairs
local table = table
local capi = {
    keygrabber = keygrabber }

local keygrabber = {}

-- Private data
local grabbers = {}
local keygrabbing = false


local function grabber(mod, key, event)
    for _, keygrabber_function in ipairs(grabbers) do
        -- continue if the grabber explicitly returns false
        if keygrabber_function(mod, key, event) ~= false then
            break
        end
    end
end

--- Stop grabbing the keyboard for the provided callback.
-- When no callback is given, the last grabber gets removed (last one added to
-- the stack).
-- @param g The key grabber that must be removed.
function keygrabber.stop(g)
    for i, v in ipairs(grabbers) do
        if v == g then
            table.remove(grabbers, i)
            break
        end
    end
    -- Stop the global key grabber if the last grabber disappears from stack.
    if #grabbers == 0 then
        keygrabbing = false
        capi.keygrabber.stop()
    end
end

---
-- Grab keyboard input and read pressed keys, calling the least callback
-- function from the stack at each keypress, until the stack is empty.
--
-- Calling run with the same callback again will bring the callback
-- to the top of the stack.
--
-- The callback function receives three arguments:
--
-- * a table containing modifiers keys
-- * a string with the pressed key
-- * a string with either "press" or "release" to indicate the event type
--
-- A callback can return `false` to pass the events to the next
-- keygrabber in the stack.
-- @param g The key grabber callback that will get the key events until it will be deleted or a new grabber is added.
-- @return the given callback `g`.
-- @usage
-- -- The following function can be bound to a key, and be used to resize a
-- -- client using the keyboard.
--
-- function resize(c)
--   local grabber
--   grabber = awful.keygrabber.run(function(mod, key, event)
--     if event == "release" then return end
--
--     if     key == 'Up'    then c:relative_move(0, 0, 0, 5)
--     elseif key == 'Down'  then c:relative_move(0, 0, 0, -5)
--     elseif key == 'Right' then c:relative_move(0, 0, 5, 0)
--     elseif key == 'Left'  then c:relative_move(0, 0, -5, 0)
--     else   awful.keygrabber.stop(grabber)
--     end
--   end)
-- end
function keygrabber.run(g)
    -- Remove the grabber if it is in the stack.
    keygrabber.stop(g)
    -- Record the grabber that has been added most recently.
    table.insert(grabbers, 1, g)
    -- Start the keygrabber if it is not running already.
    if not keygrabbing then
        keygrabbing = true
        capi.keygrabber.run(grabber)
    end
    return g
end

return keygrabber

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
