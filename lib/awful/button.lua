---------------------------------------------------------------------------
--- Create easily new buttons objects ignoring certain modifiers.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2018 Emmanuel Lepage Vallee
-- @copyright 2009 Julien Danjou
-- @inputmodule awful.button
---------------------------------------------------------------------------

-- Grab environment we need
local setmetatable = setmetatable
local ipairs = ipairs
local capi = { button = button, root = root }
local gmath = require("gears.math")
local gtable = require("gears.table")

local button = { mt = {} }

-- Due to non trivial abuse or `pairs` in older code, the private data cannot
-- be stored in the object itself without creating subtle bugs. This cannot be
-- fixed internally because the default `rc.lua` uses `gears.table.join`, which
-- is affected.
--TODO v6: Drop this
local reverse_map = setmetatable({}, {__mode="k"})

--- Modifiers to ignore.
--
-- By default this is initialized as `{ "Lock", "Mod2" }`
-- so the `Caps Lock` or `Num Lock` modifier are not taking into account by awesome
-- when pressing keys.
--
-- @table ignore_modifiers
local ignore_modifiers = { "Lock", "Mod2" }

--- The mouse buttons names.
--
-- It can be used instead of the button ids.
--
-- @table names
button.names = {
    LEFT        = 1,-- The left mouse button.
    MIDDLE      = 2,-- The scrollwheel button.
    RIGHT       = 3,-- The context menu button.
    SCROLL_UP   = 4,-- A scroll up increment.
    SCROLL_DOWN = 5,-- A scroll down increment.
}

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

--- The mouse button identifier.
--
-- ![Mouse buttons](../images/mouse.svg)
--
-- @property button
-- @param integer

--- The button description.
--
-- @property description
-- @param string

--- The button name.
--
-- @property name
-- @param string

--- The button group.
--
-- @property group
-- @param string

--- The callback when this button is pressed.
--
-- @property on_press
-- @param function

--- The callback when this button is released.
--
-- @property on_release
-- @param function

--- Execute this mousebinding.
-- @function button:trigger

function button:set_button(b)
    for _, v in ipairs(self) do
        v.button = b
    end
end

function button:set_modifiers(mod)
    local subsets = gmath.subsets(ignore_modifiers)
    for k, set in ipairs(subsets) do
        self[k].modifiers = gtable.join(mod, set)
    end
end

for _, prop in ipairs { "description", "group", "on_press", "on_release", "name" } do
    button["get_"..prop] = function(self)
        return reverse_map[self][prop]
    end
    button["set_"..prop] = function(self, value)
        reverse_map[self][prop] = value
    end
end

function button:get_button()
    return self[1].button
end

function button:trigger()
    local data = reverse_map[self]

    local press = data.weak_content.press

    if press then
        press()
    end

    local release = data.weak_content.release

    if release then
        release()
    end
end

function button:get_has_root_binding()
    return capi.root.has_button(self)
end

local function index_handler(self, k)
    if button["get_"..k] then
        return button["get_"..k](self)
    end

    if type(button[k]) == "function" then
        return button[k]
    end

    local data = reverse_map[self]
    assert(data)

    if data[k] ~= nil then
        return data[k]
    else
        return data.weak_content[k]
    end
end

local function newindex_handler(self, key, value)
    if button["set_"..key] then
        return button["set_"..key](self, value)
    end

    local data = reverse_map[self]
    assert(data)

    if data.weak_content[key] ~= nil then
        data.weak_content[key] = value
    else
        data[key] = value
    end
end

local obj_mt = {
    __index    = index_handler,
    __newindex = newindex_handler
}

--- Create a new button to use as binding.
--
-- @constructorfct awful.button
-- @tparam table mod A list of modifier keys.  Valid modifiers are:
--  `Any`, `Mod1`, Mod2`, `Mod3`, `Mod4`, `Mod5`, `Shift`, `Lock` and `Control`.
--  This argument is (**mandatory**).
-- @tparam number button The mouse button (it is recommended to use the
--  `awful.button.names` constants.
-- @tparam function press Callback for when the button is pressed.
-- @tparam function release Callback for when the button is released.
-- @treturn table An `awful.button` object.

--- Create a new button to use as binding.
--
-- @constructorfct2 awful.button
-- @tparam table args
-- @tparam table args.modifiers A list of modifier keys.  Valid modifiers are:
--  `Any`, `Mod1`, Mod2`, `Mod3`, `Mod4`, `Mod5`, `Shift`, `Lock` and `Control`.
--  This argument is (**mandatory**).
-- @tparam number args.button The mouse button (it is recommended to use the
--  `awful.button.names` constants.
-- @tparam function args.on_press Callback for when the button is pressed.
-- @tparam function args.on_release Callback for when the button is released.
-- @treturn table An `awful.button` object.

local function new_common(mod, _button, press, release)
    local ret = {}
    local subsets = gmath.subsets(ignore_modifiers)
    for _, set in ipairs(subsets) do
        ret[#ret + 1] = capi.button({ modifiers = gtable.join(mod, set),
                                      button = _button })
        if press then
            ret[#ret]:connect_signal("press", function(_, ...) press(...) end)
        end
        if release then
            ret[#ret]:connect_signal("release", function (_, ...) release(...) end)
        end
    end

    reverse_map[ret] = {
        -- Use weak tables to let Lua 5.1 and Luajit GC the `awful.buttons`,
        -- Lua 5.3 is smart enough to figure this out.
        weak_content = setmetatable({
            press   = press,
            release = release,
        }, {__mode = "v"}),
        _is_capi_button = false
    }

    return setmetatable(ret, obj_mt)
end

function button.new(args, _button, press, release)
    -- Assume this is the new constructor.
    if not _button then
        assert(not (press or release), "Calling awful.button() requires a button name")
        return new_common(
            args.modifiers,
            args.button,
            args.on_press,
            args.on_release
        )
    else
        return new_common(args, _button, press, release)
    end
end

function button.mt:__call(...)
    return button.new(...)
end

return setmetatable(button, button.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
