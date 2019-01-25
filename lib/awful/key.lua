---------------------------------------------------------------------------
--- Create easily new key objects ignoring certain modifiers.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @module awful.key
---------------------------------------------------------------------------

-- Grab environment we need
local setmetatable = setmetatable
local ipairs = ipairs
local capi = { key = key, root = root, awesome = awesome }
local gmath = require("gears.math")
local gtable = require("gears.table")

local key = { mt = {}, hotkeys = {} }

--- Modifiers to ignore.
-- By default this is initialized as { "Lock", "Mod2" }
-- so the Caps Lock or Num Lock modifier are not taking into account by awesome
-- when pressing keys.
-- @name awful.key.ignore_modifiers
-- @class table
key.ignore_modifiers = { "Lock", "Mod2" }

--- Convert the modifiers into pc105 key names
local conversion = nil

local function generate_conversion_map()
    if conversion then return conversion end

    local mods = capi.awesome._modifiers
    assert(mods)

    conversion = {}

    for mod, keysyms in pairs(mods) do
        for _, keysym in ipairs(keysyms) do
            assert(keysym.keysym)
            conversion[mod] = conversion[mod] or keysym.keysym
            conversion[keysym.keysym] = mod
        end
    end

    return conversion
end

capi.awesome.connect_signal("xkb::map_changed"  , function() conversion = nil end)

--- Execute a key combination.
-- If an awesome keybinding is assigned to the combination, it should be
-- executed.
--
-- To limit the chances of accidentally leaving a modifier key locked when
-- calling this function from a keybinding, make sure is attached to the
-- release event and not the press event.
--
-- @see root.fake_input
-- @tparam table mod A modified table. Valid modifiers are: Any, Mod1,
--   Mod2, Mod3, Mod4, Mod5, Shift, Lock and Control.
-- @tparam string k The key
function key.execute(mod, k)
    local modmap = generate_conversion_map()
    local active = capi.awesome._active_modifiers

    -- Release all modifiers
    for _, m in ipairs(active) do
        assert(modmap[m])
        root.fake_input("key_release", modmap[m])
    end

    for _, v in ipairs(mod) do
        local m = modmap[v]
        if m then
            root.fake_input("key_press", m)
        end
    end

    root.fake_input("key_press"  , k)
    root.fake_input("key_release", k)

    for _, v in ipairs(mod) do
        local m = modmap[v]
        if m then
            root.fake_input("key_release", m)
        end
    end

    -- Restore the previous modifiers all modifiers. Please note that yes,
    -- there is a race condition if the user was fast enough to release the
    -- key during this operation.
    for _, m in ipairs(active) do
        root.fake_input("key_press", modmap[m])
    end
end

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
-- @tparam[opt] function release Callback for when the key is released.
-- @tparam table data User data for key,
-- for example {description="select next tag", group="tag"}.
-- @treturn table A table with one or several key objects.
function key.new(mod, _key, press, release, data)
    if type(release)=='table' then
        data=release
        release=nil
    end
    local ret = {}
    local subsets = gmath.subsets(key.ignore_modifiers)
    for _, set in ipairs(subsets) do
        ret[#ret + 1] = capi.key({ modifiers = gtable.join(mod, set),
                                   key = _key })
        if press then
            ret[#ret]:connect_signal("press", function(_, ...) press(...) end)
        end
        if release then
            ret[#ret]:connect_signal("release", function(_, ...) release(...) end)
        end
    end

    -- append custom userdata (like description) to a hotkey
    data = data and gtable.clone(data) or {}
    data.mod = mod
    data.key = _key
    data.press = press
    data.release = release
    table.insert(key.hotkeys, data)
    data.execute = function(_) key.execute(mod, _key) end

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
        if not gtable.hasitem(pressed_mod, m) then
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
