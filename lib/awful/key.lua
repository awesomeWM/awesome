---------------------------------------------------------------------------
--- Create easily new key objects ignoring certain modifiers.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2018 Emmanuel Lepage Vallee
-- @inputmodule awful.key
---------------------------------------------------------------------------

-- Grab environment we need
local setmetatable = setmetatable
local ipairs = ipairs
local capi = { key = key, root = root, awesome = awesome }
local gmath = require("gears.math")
local gtable = require("gears.table")
local gdebug = require("gears.debug")
local gobject = require("gears.object")

--- The keyboard key used to trigger this keybinding.
--
-- It can be the key symbol, such as `space`, the character, such as ` ` or the
-- keycode such as `#65`.
--
-- @property key
-- @param string

--- A group of keys.
--
-- The valid keygroups are:
--
-- * **numrow**: The row above the letters in the US PC-105/PC-104 keyboards
--   and its derivative. This is usually the number 1-9 followed by 0.
-- * **arrows**: The Left/Right/Top/Bottom keys usually located right of the
--   spacebar.
--
-- @property keygroup

--- The table of modifier keys.
--
-- A modifier, such as `Control` are a predetermined set of keys that can be
-- used to implement keybindings. Note that this list is fix and cannot be
-- extended using random key names, code or characters.
--
-- Common modifiers are:
--
-- <table class='widget_list' border=1>
--  <tr style='font-weight: bold;'>
--   <th align='center'>Name</th>
--   <th align='center'>Description</th>
--  </tr>
--  <tr><td>Mod1</td><td>Usually called Alt on PCs and Option on Macs</td></tr>
--  <tr><td>Mod4</td><td>Also called Super, Windows and Command ⌘</td></tr>
--  <tr><td>Mod5</td><td>Also called AltGr or ISO Level 3</td></tr>
--  <tr><td>Shift</td><td>Both left and right shift keys</td></tr>
--  <tr><td>Control</td><td>Also called CTRL on some keyboards</td></tr>
-- </table>
--
-- Please note that Awesome ignores the status of "Lock" and "Mod2" (Num Lock).
--
-- @property modifiers
-- @tparam table modifiers

--- The key description.
--
-- This is used, for example, by the `awful.hotkey_popup`.
--
-- @property description
-- @param string

--- The key name.
--
-- This can be useful when searching for keybindings by keywords.
--
-- @property name
-- @param string

--- The key group.
--
-- This is used, for example, by the `awful.hotkey_popup`.
--
-- @property group
-- @param string

--- The callback when this key is pressed.
--
-- @property on_press
-- @param function

--- The callback when this key is released.
--
-- @property on_release
-- @param function

local key = { mt = {}, hotkeys = {} }

local reverse_map = setmetatable({}, {__mode="k"})

function key:set_key(k)
    for _, v in ipairs(self) do
        v.key = k
    end
end

function key:get_key()
    return self[1].key
end

function key:set_modifiers(mod)
    local subsets = gmath.subsets(key.ignore_modifiers)
    for k, set in ipairs(subsets) do
        self[k].modifiers = gtable.join(mod, set)
    end
end

for _, prop in ipairs { "description", "group", "on_press", "on_release", "name" } do
    key["get_"..prop] = function(self)
        return reverse_map[self][prop]
    end
    key["set_"..prop] = function(self, value)
        reverse_map[self][prop] = value
    end
end

--- Execute this keybinding.
--
-- @method :trigger

function key:trigger()
    local data = reverse_map[self]
    if data.on_press then
        data.on_press()
    end

    if data.on_release then
        data.on_release()
    end
end

function key:get_has_root_binding()
    return capi.root.has_key(self)
end

-- This is used by the keygrabber and prompt to identify valid awful.key
-- objects. It *cannot* be put directly in the object since `capi` uses a lot
-- of `next` internally and fixing that would suck more.
function key:get__is_awful_key()
    return true
end

local function index_handler(self, k)
    if key["get_"..k] then
        return key["get_"..k](self)
    end

    if type(key[k]) == "function" then
        return key[k]
    end

    local data = reverse_map[self]
    assert(data)

    return data[k]
end

local function newindex_handler(self, k, value)
    if key["set_"..k] then
        return key["set_"..k](self, value)
    end

    local data = reverse_map[self]
    assert(data)

    data[k] = value
end

local obj_mt = {
    __index    = index_handler,
    __newindex = newindex_handler
}

--- Modifiers to ignore.
-- By default this is initialized as `{ "Lock", "Mod2" }`
-- so the Caps Lock or Num Lock modifier are not taking into account by awesome
-- when pressing keys.
-- @name awful.key.ignore_modifiers
-- @class table
key.ignore_modifiers = { "Lock", "Mod2" }

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
-- @deprecated awful.key.execute
function key.execute(mod, k)
    gdebug.deprecate("Use `awful.keyboard.emulate_key_combination` or "..
        "`my_key:trigger()` instead of `awful.key.execute()`",
        {deprecated_in=5}
    )

    require("awful.keyboard").emulate_key_combination(mod, k)
end

--- Create a new key binding.
--
-- @constructorfct2 awful.key
-- @tparam table args
-- @tparam function args.key The key to trigger an event. It can be the character
--  itself of `#+keycode` (**mandatory**).
-- @tparam function args.modifiers A list of modifier keys.  Valid modifiers are:
--  `Any`, `Mod1`, Mod2`, `Mod3`, `Mod4`, `Mod5`, `Shift`, `Lock` and `Control`.
--  This argument is (**mandatory**).
-- @tparam function args.on_press Callback for when the key is pressed.
-- @tparam function args.on_release Callback for when the key is released.

--- Create a new key binding (alternate constructor).
--
-- @tparam table mod A list of modifier keys.  Valid modifiers are: `Any`,
--  `Mod1`, `Mod2`, `Mod3`, `Mod4`, `Mod5`, `Shift`, `Lock` and `Control`.
-- @tparam string _key The key to trigger an event. It can be the character
--  itself of `#+keycode`.
-- @tparam function press Callback for when the key is pressed.
-- @tparam[opt] function release Callback for when the key is released.
-- @tparam[opt] table data User data for key,
-- for example {description="select next tag", group="tag"}.
-- @treturn table A table with one or several key objects.
-- @constructorfct awful.key

local function new_common(mod, _keys, press, release, data)
    if type(release)=='table' then
        data=release
        release=nil
    end

    local ret = {}
    local subsets = gmath.subsets(key.ignore_modifiers)
    for _, key_pair in ipairs(_keys) do
        for _, set in ipairs(subsets) do
            local sub_key = capi.key {
                modifiers = gtable.join(mod, set),
                key       = key_pair[1]
            }

            sub_key._private._legacy_convert_to = ret

            sub_key:connect_signal("press", function(_, ...)
                if ret.on_press then
                    if key_pair[2] ~= nil then
                        ret.on_press(key_pair[2], ...)
                    else
                        ret.on_press(...)
                    end
                end
            end)

            sub_key:connect_signal("release", function(_, ...)
                if ret.on_release then
                    if key_pair[2] ~= nil then
                        ret.on_release(key_pair[2], ...)
                    else
                        ret.on_release(...)
                    end
                end
            end)

            ret[#ret + 1] = sub_key
        end
    end

    -- append custom userdata (like description) to a hotkey
    data = data and gtable.clone(data) or {}
    data.mod = mod
    data.keys = _keys
    data.on_press = press
    data.on_release = release
    data._is_capi_key = false
    assert((not data.key) or type(data.key) == "string")
    table.insert(key.hotkeys, data)
    data.execute = function(_)
        assert(#_keys == 1, "key:execute() makes no sense for groups")
        key.execute(mod, _keys[1])
    end

    -- Store the private data
    reverse_map[ret] = data

    --WARNING this object needs to expose only ordered keys for legacy reasons.
    -- All other properties needs to be fully handled by the meta table and never
    -- be stored directly in the object.

    return setmetatable(ret, obj_mt)
end

local keygroups = {
    -- Left: the keycode in a format which regular awful.key understands.
    -- Right: the argument of the function ran upon executing the key binding.
    numrow = {},
    arrows = {
        {"Left"  , "Left"  },
        {"Right" , "Right" },
        {"Up"    , "Up"    },
        {"Down",   "Down"  },
    }
}

-- Technically, this isn't very popular, but we have been doing this for 12
-- years and nobody complained too loudly.
for i = 1, 10 do
    table.insert(keygroups.numrow, {"#" .. i + 9, i == 10 and 0 or i})
end

-- Allow key objects to provide more than 1 key.
--
-- Some "groups" like arrows, the numpad, F-keys or numrow "belong together"
local function get_keys(args)
    if not args.keygroup then return {{args.key}} end

    -- Make sure nothing weird happens.
    assert(
        not args.key,
        "Please provide either the `key` or `keygroup` property, not both"
    )

    assert(keygroups[args.keygroup], "Please provide a valid keygroup")
    return keygroups[args.keygroup]
end

function key.new(args, _key, press, release, data)
    -- Assume this is the new constructor.
    if not _key then
        assert(not (press or release or data), "Calling awful.key() requires a key name")

        local keys = get_keys(args)

        return new_common(
            args.modifiers,
            keys,
            args.on_press,
            args.on_release,
            args
        )
    else
        return new_common(args, {{_key}}, press, release, data)
    end
end

--- Compare a key object with modifiers and key.
-- @param _key The key object.
-- @param pressed_mod The modifiers to compare with.
-- @param pressed_key The key to compare with.
-- @staticfct awful.key.match
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

gobject.properties(capi.key, {
    auto_emit = true,
})

return setmetatable(key, key.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
