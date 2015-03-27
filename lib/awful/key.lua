---------------------------------------------------------------------------
--- Create easily new key objects ignoring certain modifiers.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.key
---------------------------------------------------------------------------

-- Grab environment we need
local setmetatable = setmetatable
local ipairs = ipairs
local capi = { key = key }
local util = require("awful.util")

local key = { mt = {} }

--- Modifiers to ignore.
-- By default this is initialized as { "Lock", "Mod2" }
-- so the Caps Lock or Num Lock modifier are not taking into account by awesome
-- when pressing keys.
-- @name ignore_modifiers
-- @class table
local ignore_modifiers = { "Lock", "Mod2" }

--- Create a new key to use as binding.
-- This function is useful to create several keys from one, because it will use
-- the ignore_modifier variable to create several keys with and without the
-- ignored modifiers activated.
-- For example if you want to ignore CapsLock in your keybinding (which is
-- ignored by default by this function), creating a key binding with this
-- function will return 2 key objects: one with CapsLock on, and another one
-- with CapsLock off.
-- @see key.key
-- @tparam table mod A list of modifier keys.  Valid modifiers are: Any, Mod1,
--   Mod2, Mod3, Mod4, Mod5, Shift, Lock and Control.
-- @tparam string _key The key to trigger an event.
-- @tparam function press Callback for when the key is pressed.
-- @tparam function release Callback for when the key is released.
-- @treturn table A table with one or several key objects.
function key.new(mod, _key, press, release)
    local ret = {}
    local subsets = util.subsets(ignore_modifiers)
    for _, set in ipairs(subsets) do
        ret[#ret + 1] = capi.key({ modifiers = util.table.join(mod, set),
                                   key = _key })
        if press then
            ret[#ret]:connect_signal("press", function(kobj, ...) press(...) end)
        end
        if release then
            ret[#ret]:connect_signal("release", function(kobj, ...) release(...) end)
        end
    end
    return ret
end

--- Compare a key object with modifiers and key.
-- @param _key The key object.
-- @param pressed_mod The modifiers to compare with.
-- @param pressed_key The key to compare with.
function key.match(_key, pressed_mod, pressed_key)
    -- First, compare key.
    if pressed_key ~= _key.key then return false end
    -- Then, compare mod
    local mod = _key.modifiers
    -- For each modifier of the key object, check that the modifier has been
    -- pressed.
    for _, m in ipairs(mod) do
        -- Has it been pressed?
        if not util.table.hasitem(pressed_mod, m) then
            -- No, so this is failure!
            return false
        end
    end
    -- If the number of pressed modifier is ~=, it is probably >, so this is not
    -- the same, return false.
    return #pressed_mod == #mod
end

function key.mt:__call(...)
    return key.new(...)
end

return setmetatable(key, key.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
