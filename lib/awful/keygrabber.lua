---------------------------------------------------------------------------
--- A keyboard grabbing and transaction object.
--
-- This module allows to grab all keyboard inputs until stopped. It is used
-- to gather input data (such as in `awful.prompt`), implement VI-like keybindings
-- or multi key transactions such as emulating the Alt+Tab behavior.
--
-- Note that this module has been redesigned in Awesome 4.3 to be object oriented
-- and stateful. The use of the older global keygrabbing API is discouraged
-- going forward since it had problem with recursive keygrabbers and required
-- a lot of boiler plate code to get anything done.
--
-- Using keygrabber as transactions
-- --------------------------------
--
-- The transactional keybindings are keybindings that start with a normal
-- keybindings, but only end when a (mod)key is released. In the classic
-- "Alt+Tab" transaction, first pressing "Alt+Tab" will display a popup listing
-- all windows (clients). This popup will only disappear when "Alt" is released.
--
-- @DOC_text_awful_keygrabber_alttab_EXAMPLE@
--
-- In that example, because `export_keybindings` is set to `true`, pressing
-- `alt+tab` or `alt+shift+tab` will start the transaction even if the
-- keygrabber is not (yet) running.
--
-- Using keygrabber for modal keybindings (VI like)
-- ------------------------------------------------
--
-- VI-like modal keybindings are trigerred by a key, like `Escape`, followed by
-- either a number, an adjective (or noun) and closed by a verb. For example
-- `<Escape>+2+t+f` could mean "focus (f) the second (2) tag (t)".
-- `<Escape>+2+h+t+f` would "focus (f) two (2) tags (t) to the right (h)".
--
-- Here is a basic implementation of such a system. Note that the action
-- functions themselves are not implemented to keep the example size and
-- complexity to a minimum. The implementation is just if/elseif of all action
-- and the code can be found in the normal `rc.lua` keybindings section:
--
-- @DOC_text_awful_keygrabber_vimode_EXAMPLE@
--
-- Using signals
-- -------------
--
-- When the keygrabber is running, it will emit signals on each event. The
-- format is "key_name".."::".."pressed_or_released". For example, to attach
-- a callback to `Escape` being pressed, do:
--
--    mykeygrabber:connect_signal("Escape::pressed", function(self, modifiers, modset)
--        print("Escape called!")
--    end)
--
-- The `self` argument is the keygrabber instance. The `modifiers` is a list of
-- all the currently pressed modifiers and the `modset` is the same table, but
-- with the modifiers as keys instead of value. It allow to save time by
-- checking `if modset.Mod4 then ... end` instead of looping into the `modifiers`
-- table or using `gears.table`.
--
-- @author dodo
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2012 dodo
-- @copyright 2017 Emmanuel Lepage Vallee
-- @classmod awful.keygrabber
---------------------------------------------------------------------------

local ipairs = ipairs
local table = table
local gdebug = require('gears.debug')
local akey = require("awful.key")
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local gtable = require("gears.table")
local gobject = require("gears.object")
local gtimer = require("gears.timer")
local glib = require("lgi").GLib
local capi = { keygrabber = keygrabber, root = root }

local keygrab = {}

-- Private data
local grabbers = {}
local keygrabbing = false

local keygrabber = {
    object = {}
}

-- Instead of checking for every modifiers, check the key directly.
--FIXME This is slightly broken but still good enough for `mask_modkeys`
local conversion = {
    Super_L   = "Mod4",
    Control_L = "Control",
    Shift_L   = "Shift",
    Alt_L     = "Mod1",
    Super_R   = "Mod4",
    Control_R = "Control",
    Shift_R   = "Shift",
    Alt_R     = "Mod1",
}

--BEGIN one day create a proper API to add and remove keybindings at runtime.
-- Doing it this way is horrible.

-- This list of keybindings to add in the next event loop cycle.
local delay_list = {}

local function add_root_keybindings(self, list)
    assert(
        list, "`add_root_keybindings` needs to be called with a list of keybindings"
    )

    local was_started = #delay_list > 0

    -- When multiple `awful.keygrabber` objects are created in `rc.lua`, avoid
    -- unpacking and repacking all keys for each instance and instead merge
    -- everything into one operation. In not so extreme cases, not doing so
    -- would slow down `awesome.restart()` by a small, but noticeable amount
    -- of time.
    gtable.merge(delay_list, list)

    -- As of Awesome v4.3, `root.keys()` is an all or nothing API and there
    -- isn't a standard mechanism to add and remove keybindings at runtime
    -- without replacing the full list. Given `rc.lua` sets this list, not
    -- using a delayed call would cause all `awful.keygrabber` created above
    -- `root.keys(globalkeys)` to be silently overwritten. --FIXME v5
    if not was_started then
        gtimer.delayed_call(function()
            local ret = {}

            for _, v in ipairs(delay_list) do
                local mods, key, press, release, description = unpack(v)

                if press then
                    local old_press = press
                    press = function(...)
                        self:start()
                        old_press(...)
                    end
                end

                if release then
                    local old_release = release
                    release = function(...)
                        self:start()
                        old_release(...)
                    end
                end

                table.insert(ret, akey(mods, key, press, release, description))
            end

            -- Wow...
            capi.root.keys(gtable.join( capi.root.keys(), unpack(ret) ))

            delay_list = {}
        end)
    end
end

--END hack

local function grabber(mod, key, event)
    for _, keygrabber_function in ipairs(grabbers) do
        -- continue if the grabber explicitly returns false
        if keygrabber_function(mod, key, event) ~= false then
            break
        end
    end
end

local function runner(self, modifiers, key, event)
    -- Stop the keygrabber with the `stop_key`
    if key == self.stop_key
        and event == self.stop_event and self.stop_key then
        self:stop(key, modifiers)
        return false
    end

    -- Stop when only a subset of keys are allowed and it isn't one of them.
    if self._private.allowed_keys and not self._private.allowed_keys[key] then
        self:stop(key, modifiers)
        return false
    end

    -- Support multiple stop keys
    if type(self.stop_key) == "table" and event == self.stop_event then
        for _, k in ipairs(self.stop_key) do
            if k == key then
                self:stop(k, modifiers)
                return false
            end
        end
    end

    local is_modifier = conversion[key] ~= nil

    -- Reset the inactivity timer on each events.
    if self._private.timer and self._private.timer.started then
        self._private.timer:again()
    end

    -- Lua strings are bytes, to handle UTF-8, use GLib
    local seq_len = glib.utf8_strlen(self.sequence, -1)

    -- Record the key sequence
    if key == "BackSpace" and seq_len > 0 then
        self.sequence = glib.utf8_substring(self.sequence, 0, seq_len - 2)
    elseif glib.utf8_strlen(key, -1) == 1 and  event == "release" then
        self.sequence = self.sequence..key
    end

    -- Convert index array to hash table
    local mod = {}
    for _, v in ipairs(modifiers) do mod[v] = true end

    -- Emit some signals so the user can connect to a single type of event.
    self:emit_signal(key.."::"..event, modifiers, mod)

    local filtered_modifiers = {}

    -- User defined cases
    if self._private.keybindings[key] and event == "press" then
        -- Remove caps and num lock
        for _, m in ipairs(modifiers) do
            if not gtable.hasitem(akey.ignore_modifiers, m) then
                table.insert(filtered_modifiers, m)
            end
        end

        for _,v in ipairs(self._private.keybindings[key]) do
            if #filtered_modifiers == #v[1] then
                local match = true
                for _,v2 in ipairs(v[1]) do
                    match = match and mod[v2]
                end
                if match then
                    v[3](self)

                    if self.mask_event_callback ~= false then
                        return
                    end
                end
            end
        end
    end

    -- Do not call the callbacks on modkeys
    if is_modifier and self.mask_modkeys then
        return
    end

    if self.keypressed_callback and event == "press" then
        self.keypressed_callback(self, modifiers, key, event)
    elseif self.keyreleased_callback and event == "release" then
        self.keyreleased_callback(self, modifiers, key, event)
    end
end

--- Stop grabbing the keyboard for the provided callback.
--
-- When no callback is given, the last grabber gets removed (last one added to
-- the stack).
--
-- @param[opt] g The key grabber that must be removed.
-- @deprecated awful.keygrabber.stop
function keygrab.stop(g)
    -- If `run` is used directly and stop() is called with `g==nil`, get the
    -- first keygrabber.
    g = g
        or keygrab.current_instance and keygrab.current_instance.grabber
        or grabbers[#grabbers]

    for i, v in ipairs(grabbers) do
        if v == g then
            table.remove(grabbers, i)
            break
        end
    end

    if g and keygrab.current_instance and keygrab.current_instance.grabber == g then
        keygrab.current_instance = nil
    end

    -- Stop the global key grabber if the last grabber disappears from stack.
    if #grabbers == 0 then
        keygrabbing = false
        capi.keygrabber.stop()
    end
end

--- The keygrabber timeout.
--
-- @DOC_text_awful_keygrabber_timeout_EXAMPLE@
--
-- @property timeout
-- @see gears.timer
-- @see timeout_callback

--- The key on which the keygrabber listen to terminate itself.
--
-- When this is set, the running keygrabber will quit when [one of] the stop
-- key event occurs.
--
-- By default, the event is `press`. It is common for use case like the
-- `awful.prompt` where `return` (enter) will terminate the keygrabbing. Using
-- `release` as an event is more appropriate when the keygrabber is tied to a
-- modifier key. For example, an Alt+Tab implementation stops when `mod1` (Alt)
-- is released.
--
-- It can also be a table containing many keys (as values).
--
-- @DOC_text_awful_keygrabber_stop_keys_EXAMPLE@
--
-- @DOC_text_awful_keygrabber_stop_key_EXAMPLE@
--
-- Please note that modkeys are not accepted as `stop_key`s. You have to use
-- their corresponding key names such as `Control_L`.
--
-- @property stop_key
-- @param[opt=nil] string|table stop_key
-- @see stop_event

--- The event on which the keygrabbing will be terminated.
--
-- the valid values are:
--
-- * "press" (default)
-- * "release"
--
-- @property stop_event
-- @param string
-- @see stop_key

--- Whether or not to execute the key press/release callbacks when keybindings are called.
--
-- When this property is set to false, the `keyreleased_callback` and
-- `keypressed_callback` callbacks will be executed even when the event was
-- consumed by a `keybinding`.
--
-- By default, keybindings block those callbacks from being executed.
--
-- @property mask_event_callback
-- @param[opt=true] boolean
-- @see keybindings
-- @see keyreleased_callback
-- @see keypressed_callback

--- Do not call the callbacks on modifier keys (like `Control` or `Mod4`) events.
-- @property mask_modkeys
-- @param[opt=false] boolean
-- @see mask_event_callback

--- Export all keygrabber keybindings as root (global) keybindings.
--
-- When this is enabled, calling all of the keygrabber object `keybinding`s will
-- will create root `awful.key` and will automatically starts the grabbing.
--
-- Use this with caution. If many keygrabber or "real" root keybindings are set
-- on the same key combination, they are all executed and there is almost no
-- safe way to undo that. Make sure the `keygrabber` that use this option
-- have a single instance.
--
-- @property export_keybindings
-- @param[opt=false] boolean

--- The root (global) keybinding to start this keygrabber.
--
-- Instead of adding an entry to `root.keys` or `rc.lua` `globalkeys` section,
-- this property can be used to take care of everything. This way, it becomes
-- easier to package modules using keygrabbers.
--
-- @DOC_text_awful_keygrabber_root_keybindings_EXAMPLE@
--
-- @property root_keybindings
-- @see export_keybindings
-- @see keybindings

--- The keybindings associated with this keygrabber.
--
-- The keybindings syntax is the same as `awful.key` or `awful.prompt.hooks`. It
-- consists of a table with 4 entries.
--
-- * `mods` A table with modifier keys, such as `shift`, `mod4`, `mod1` (alt) or
--  `control`.
-- * `key` The key name, such as `left` or `f`
-- * `callback` A function that will be called when the key combination is
--  pressed.
-- * `description` A table various metadata to be used for `awful.hotkeys_popup`.
--
-- @property keybindings
-- @param table
-- @see export_keybindings
-- @see root_keybindings

--- If any key is pressed that is not in this list, the keygrabber is stopped.
--
-- When defined, this property allows to define an implicit way to release the
-- keygrabber. It helps save some boilerplate code in the handler callbacks.
--
-- It is useful when a transaction only handle a limited number of keys. If
-- a key unhandled by the transaction is trigerred, the transaction is
-- canceled.
--
-- @DOC_text_awful_keygrabber_allowed_keys_EXAMPLE@
--
-- @property allowed_keys
-- @tparam[opt=nil] table|nil allowed_keys The list of keys.

--- The sequence of keys recorded since the start of the keygrabber.
--
-- In this example, the `stop_callback` is used to retrieve the final key
-- sequence.
--
-- Please note that backspace will remove elements from the sequence and
-- named keys and modifiers are ignored.
--
-- @DOC_text_awful_keygrabber_autostart_EXAMPLE@
--
-- @property sequence
-- @param string
--

--- The current (running) instance of this keygrabber.
--
-- The keygrabber instance is created when the keygrabber starts. It is an object
-- mirroring this keygrabber object, but where extra properties can be added. It
-- is useful to keep some contextual data part of the current transaction without
-- poluting the original object of having extra boilerplate code.
--
-- @tfield keygrabber current_instance
-- @see property::current_instance

--- The global signal used to track the `current_instance`.
--
-- @signal property::current_instance

--- If a keygrabber is currently running.
-- @tfield boolean is_running

--- Start the keygrabber.
--
-- Note that only a single keygrabber can be started at any one time. If another
-- keygrabber (or this one) is currently running. This method returns false.
--
-- @treturn boolean If the keygrabber was successfully started.
function keygrabber:start()
    if self.grabber or keygrab.current_instance then
        return false
    end

    self.current_instance = setmetatable({}, {
        __index = self,
        __newindex = function(tab, key, value)
            if keygrabber["set_"..key] then
                self[key](self, value)
            else
                rawset(tab, key, value)
            end
        end
    })

    self.sequence = ""

    if self.start_callback then
        self.start_callback(self.current_instance)
    end

    self.grabber = keygrab.run(function(...) return runner(self, ...) end)

    -- Ease making keygrabber that wont hang forever if no action is taken.
    if self.timeout and not self._private.timer then
        self._private.timer = gtimer {
            timeout     = self.timeout,
            single_shot = true,
            callback    = function()
                if self.timeout_callback then
                    pcall(self.timeout_callback, self)
                end
                self:stop()
            end
        }
    end

    if self._private.timer then
        self._private.timer:start()
    end

    keygrab.current_instance = self

    keygrab.emit_signal("property::current_instance", keygrab.current_instance)

    self:emit_signal("started")
end

--- Stop the keygrabber.
-- @function keygrabber:stop
function keygrabber:stop(_stop_key, _stop_mods) -- (at)function disables ldoc params
    keygrab.stop(self.grabber)

    if self.stop_callback then
        self.stop_callback(
            self.current_instance, _stop_key, _stop_mods, self.sequence
        )
    end

    keygrab.emit_signal("property::current_instance", nil)

    self.grabber = nil
    self:emit_signal("stopped")
end

--- Add a keybinding to this keygrabber.
--
-- Those keybindings will automatically start the keygrabbing when hit.
--
-- @tparam table mods A table with modifier keys, such as `shift`, `mod4`, `mod1` (alt) or
--  `control`.
-- @tparam string key The key name, such as `left` or `f`
-- @tparam function callback A function that will be called when the key
-- combination is pressed.
-- @tparam[opt] table description A table various metadata to be used for `awful.hotkeys_popup`.
-- @tparam string description.description The keybinding description
-- @tparam string description.group The keybinding group
function keygrabber:add_keybinding(mods, key, callback, description)
    self._private.keybindings[key] = self._private.keybindings[key] or {}
    table.insert(self._private.keybindings[key], {
        mods, key, callback, description
    })

    if self.export_keybindings then
        add_root_keybindings(self, {{mods, key, callback, description}})
    end
end

function keygrabber:set_root_keybindings(keys)
    add_root_keybindings(self, keys)
end

-- Turn into a set.
function keygrabber:set_allowed_keys(keys)
    self._private.allowed_keys = {}

    for _, v in ipairs(keys) do
        self._private.allowed_keys[v] = true
    end
end

--TODO v5 remove this
function keygrabber:set_release_event(event)
    -- It has never been in a release, so it can be deprecated right away
    gdebug.deprecate("release_event has been renamed to stop_event")

    self.stop_event = event
end

--TODO v5 remove this
function keygrabber:get_release_event()
    return self.stop_event
end

--- When the keygrabber starts.
-- @signal started

--- When the keygrabber stops.
-- @signal stopped

--- A function called when a keygrabber starts.
-- @callback start_callback
-- @tparam keygrabber keygrabber The keygrabber.

--- The callback when the keygrabbing stops.
--
-- @usage local function my_done_cb(self, stop_key, stop_mods, sequence)
--    -- do something
-- end
-- @tparam table self The current transaction object.
-- @tparam string stop_key The key(s) that stop the keygrabbing (if any)
-- @tparam table stop_mods The modifiers key (if any)
-- @tparam sting sequence The recorded key sequence.
-- @callback stop_callback
-- @see sequence
-- @see stop

--- The function called when the keygrabber stops because of a `timeout`.
--
-- Note that the `stop_callback` is also called when the keygrabbers timeout.
--
-- @callback timeout_callback
-- @see timeout

--- The callback function to call with mod table, key and command as arguments
-- when a key was pressed.
--
-- @callback keypressed_callback
-- @tparam table self The current transaction object.
-- @tparam table mod The current modifiers (like "Control" or "Shift").
-- @tparam string key The key name.
-- @tparam string event The event ("press" or "release").
-- @usage local function my_keypressed_cb(mod, key, command)
--    -- do something
-- end

--- The callback function to call with mod table, key and command as arguments
-- when a key was released.
-- @usage local function my_keyreleased_cb(mod, key, command)
--    -- do something
-- end
-- @callback keyreleased_callback
-- @tparam table self The current transaction object.
-- @tparam table mod The current modifiers (like "Control" or "Shift").
-- @tparam string key The key name.
-- @tparam string event The event ("press" or "release")

--- A wrapper around the keygrabber to make it easier to add keybindings.
--
-- This is similar to the `awful.prompt`, but without an actual widget. It can
-- be used to implement custom transactions such as alt+tab or keyboard menu
-- navigation.
--
-- Note that the object returned can be used as a client or global keybinding
-- callback without modification. Make sure to set `stop_key` and `stop_event`
-- to proper values to avoid permanently locking the keyboard.
--
-- @tparam table args
-- @tparam[opt="press"] string args.stop_event Release event ("press" or "release")
-- @tparam[opt=nil] string|table args.stop_key The key that has to be kept pressed.
-- @tparam table args.keybindings All keybindings.
-- @tparam[opt=-1] number args.timeout The maximum inactivity delay.
-- @tparam[opt=true] boolean args.mask_event_callback Do not call `keypressed_callback`
--  or `keyreleased_callback` when a hook is called.
-- @tparam[opt] function args.start_callback
-- @tparam[opt] function args.stop_callback
-- @tparam[opt] function args.timeout_callback
-- @tparam[opt] function args.keypressed_callback
-- @tparam[opt] function args.keyreleased_callback
-- @tparam[opt=nil] table|nil args.allowed_keys A table with all keys that
--  **wont** stop the keygrabber.
-- @tparam[opt] table args.root_keybindings The root (global) keybindings.
-- @tparam[opt=false] boolean args.export_keybindings Create root (global) keybindings.
-- @tparam[opt=false] boolean args.autostart Start the grabbing immediately
-- @tparam[opt=false] boolean args.mask_modkeys Do not call the callbacks on
--  modifier keys (like `Control` or `Mod4`) events.
-- @function awful.keygrabber
function keygrab.run_with_keybindings(args)
    args = args or {}

    local ret = gobject {
        enable_properties   = true,
        enable_auto_signals = true
    }

    rawset(ret, "_private", {})

    -- Do not use `rawset` or `_private` because it uses the `gears.object`
    -- auto signals.
    ret.sequence = ""

    gtable.crush(ret, keygrabber, true)

    gtable.crush(ret, args)

    ret._private.keybindings = {}
    ret.stop_event = args.stop_event or "press"

    -- Build the hook map
    for _,v in ipairs(args.keybindings or {}) do
        if #v >= 3 and #v <= 4 then
            local _,key,callback = unpack(v)
            if type(callback) == "function" then
                ret._private.keybindings[key] = ret._private.keybindings[key] or {}
                table.insert(ret._private.keybindings[key], v)
            else
                gdebug.print_warning(
                    "The hook's 3rd parameter has to be a function. " ..
                        gdebug.dump(v)
                )
            end
        else
            gdebug.print_warning(
                "The hook has to have at least 3 parameters. ".. gdebug.dump(v)
            )
        end
    end

    if args.export_keybindings then
        add_root_keybindings(ret, args.keybindings)
    end

    local mt = getmetatable(ret)

    -- Add syntax-sugar to call `:start()`.
    -- This allows keygrabbers object to be used as callbacks "functions".
    function mt.__call()
        ret:start()
    end

    if args.autostart then
        ret:start()
    end

    return ret
end

--- Run a grabbing function.
--
-- Calling this is equivalent to `keygrabber.run`.
--
-- @param g The key grabber callback that will get the key events until it
--   will be deleted or a new grabber is added.
-- @return the given callback `g`.
--
-- @deprecated awful.keygrabber.run
-- @see keygrabber.run

--- A lower level API to interact with the keygrabber directly.
--
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
-- * a string with either "press" or "release" to indicate the event type.
--
-- Here is the content of the modifier table:
--
-- <table>
--  <tr style='font-weight: bold;'>
--   <th align='center'>Modifier name </th>
--   <th align='center'>Key name</th>
--   <th align='center'>Alternate key name</th>
--  </tr>
--  <tr><td> Mod4</td><td align='center'> Super_L </td><td align='center'> Super_R </td></tr>
--  <tr><td> Control </td><td align='center'> Control_L </td><td align='center'> Control_R </td></tr>
--  <tr><td> Shift </td><td align='center'> Shift_L </td><td align='center'> Shift_R </td></tr>
--  <tr><td> Mod1</td><td align='center'> Alt_L </td><td align='center'> Alt_R </td></tr>
-- </table>
--
-- A callback can return `false` to pass the events to the next
-- keygrabber in the stack.
--
-- @param g The key grabber callback that will get the key events until it
--  will be deleted or a new grabber is added.
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
-- @function awful.keygrabber.run
function keygrab.run(g)
    -- Remove the grabber if it is in the stack.
    keygrab.stop(g)

    -- Record the grabber that has been added most recently.
    table.insert(grabbers, 1, g)

    -- Start the keygrabber if it is not running already.
    if not keygrabbing then
        keygrabbing = true
        capi.keygrabber.run(grabber)
    end

    return g
end

-- Implement the signal system for the keygrabber.

local signals = {}

--- Connect to a signal for all keygrabbers at once.
-- @function awful.keygrabber.connect_signal
-- @tparam string name The signal name.
-- @tparam function callback The callback.
function keygrab.connect_signal(name, callback)
    signals[name] = signals[name] or {}

    -- Use table.insert instead of signals[name][callback] = true to make
    -- the execution order stable across `emit_signal`. This avoids some
    -- heisenbugs.
    table.insert(signals[name], callback)
end

--- Disconnect to a signal for all keygrabbers at once.
-- @function awful.keygrabber.disconnect_signal
-- @tparam string name The signal name.
-- @tparam function callback The callback.
function keygrab.disconnect_signal(name, callback)
    signals[name] = signals[name] or {}

    for k, v in ipairs(signals[name]) do
        if v == callback then
            table.remove(signals[name], k)
            break
        end
    end
end

--- Emit a signal on the keygrabber module itself.
--
-- Warning, usually don't use this directly, use
-- `my_keygrabber:emit_signal(name, ...)`. This function works on the whole
-- keygrabber module, not one of its instance.
--
-- @function awful.keygrabber.emit_signal
-- @tparam string name The signal name.
-- @param ... Other arguments for the callbacks.
function keygrab.emit_signal(name, ...)
    signals[name] = signals[name] or {}

    for _, cb in ipairs(signals[name]) do
        cb(...)
    end
end

function keygrab.get_is_running()
    return keygrab.current_instance ~= nil
end

--@DOC_object_COMMON@

return setmetatable(keygrab, {
    __call = function(_, args) return keygrab.run_with_keybindings(args) end
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
