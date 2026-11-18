---------------------------------------------------------------------------
--- A generic and stateful history tracking class.
--
-- This module hosts an abstract implementation of history tracking. It can be
-- used with any datatype and hide the implementation details.
--
-- Alt+Tab like iterator for clients
-- =================================
--
--    local h = gears.history {
--        unique = true , -- Automagically deduplicate entries
--        count  = false, -- Keep track of the number of time an item was pushed
--    }
--
--    -- Listen to event to push/pop/remove objects without boilerplate code.
--    client.connect_signal("focus"   , h.push   )
--    client.connect_signal("manage"  , h.push   )
--    client.connect_signal("unmanage", h.remove )
--
--    -- Abstract way to select objects.
--    h:connect_signal("request::select", function(_, c)
--        client.focus = c
--        c:jump_to()
--    end)
--
--    -- Support multiple iterators per history object.
--    local h_i = h:iterate_newest {
--        filter = function(c) return c.screen == awful.screen.focused() end,
--    }
--
--    -- Easy to use for transactions.
--    awful.keygrabber {
--        keybindings = {
--            {{'Mod1'         }, 'Tab', h_i.select_next    },
--            {{'Mod1', 'Shift'}, 'Tab', h_i.select_previous},
--        },
--        stop_key           = 'Mod1', -- This is (left) "Alt".
--        stop_event         = 'release',
--        start_callback     = h.pause,
--        stop_callback      = h.resume,
--        export_keybindings = true,
--    }
--
--    -- Easy to iterate.
--    h.paused = true
--
--    for my_object in h:iterate_oldest() do
--        -- something
--    end
--
--    h.paused = false
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2014-2019 Emmanuel Lepage Vallee
-- @classmod gears.history
---------------------------------------------------------------------------

local gobject = require("gears.object")
local gtable  = require("gears.table")

-- rename gears.ordered_set

--TODO
-- Support filter (and make the result iterable, map-reduce chain style)
-- Make sure it can be dumped into awful.widget.common as-is
-- Keep track of the hit count of each entry (for commands)
-- Support serialization using some external gears stuff?
-- Make this the base of awful.widget.common and replace fully Radical
-- Add some way to auto generate roles mapping for the awful.widget.common
--   template
-- Replace awful.completion, somehow
-- Replace the prompt history with this

local module = {}

--- The history updating mode.
--
-- Possible values are:
--
-- * *heap*: Always keep a single instance of each entry (default)
-- * *multi*: Allow an entry to be in multiple positions
--
-- @property mode
-- @param string

--- The maximum number of entries.
--
-- Use `math.huge` for unlimited.
--
-- @property maximum_entries
-- @param[opt=math.huge] number
-- @see entry::ousted

--- Count the number of time an entry is used.
-- @property count
-- @param[opt=false] boolean

--- Is the history tracking currently paused.
--
-- When paused, all `push` will be ignored.
--
-- @property paused
-- @param boolean

--- Resume tracking.
--
-- Note that this is a function, not a method. It is safe to use directly
-- in `connect_signal` or as a callback.
--
-- @method resume

--- Pause tracking.
--
-- Note that this is a function, not a method. It is safe to use directly
-- in `connect_signal` or as a callback.
--
-- @method pause

--- Push an entry to the front (most recent) spot.
--
-- Note that this is a function, not a method. It is safe to use directly
-- in `connect_signal` or as a callback.
--
-- Note that when the history tracking is paused, calling this has no effect.
-- This allows to connect the function directly without additional boilerplate
-- to take the state into account.
--
-- **Usage for clients:**
--
--    client.connect_signal("focus"  , my_history.push )
--    client.connect_signal("manage" , my_history.push )
--
-- @method push
-- @param object Something to push.

--- Remove a value.
--
-- Note that this is a function, not a method. It is safe to use directly
-- in `connect_signal` or as a callback.
--
-- **Usage for clients:**
--
--    client.connect_signal("unmanage", my_history.remove )
--
-- @method remove
-- @param value

--- Create a new iterator that starts with the **newest** entry.
--
-- @tparam table args The arguments.
-- @tparam[opt=true] boolean args.rotate Define if the `next` and `previous`
--  iterator methods will rotate once the reach the boundary. This does **not**
--  affect the behavior when iterating using a `for` loop.
-- @tparam[opt=nil] function args.filter A filter function. It takes the object
--  as sole parameter and it will allowed if the function returns true.
--  Otherwise, the next element will be passed to the filter until it returns
--  true.
-- @method iterate_newest
-- @usage my_history.paused = true
-- for my_object in my_history:iterate_newest() do
--     -- something
-- end
-- my_history.paused = false

--- Create a new iterator that starts with the **oldest** entry.
--
-- @tparam table args The arguments.
-- @tparam[opt=true] boolean args.rotate Define if the `next` and `previous`
--  iterator methods will rotate once the reach the boundary. This does **not**
--  affect the behavior when iterating using a `for` loop.
-- @tparam[opt=nil] function args.filter A filter function. It takes the object
--  as sole parameter and it will allowed if the function returns true.
--  Otherwise, the next element will be passed to the filter until it returns
--  true.
-- @method iterate_oldest
-- @usage my_history.paused = true
-- for my_object in my_history:iterate_oldest() do
--     -- something
-- end
-- my_history.paused = false

--- Returns the oldest object tracked by this history.
-- @return The oldest object.
-- @method oldest

--- Returns the newest object tracked by this history.
-- @return The newest object.
-- @method newest

--- Emitted when the history tracking is paused.
-- @signal paused

--- Emitted when the history tracking is resumed.
-- @signal resumed

--- Emitted when the iterator `:select()` method is called.
--
-- Calling `:select()` itself does nothing beside emitting this signal. It is
-- up to the API user to implement a handler for it.
--
-- **Usage for clients:**
--
--    my_history:connect_signal("request::select", function(_, c)
--        client.focus = c
--        c:jump_to()
--    end)
--
-- @signal request::select
-- @param o The object.

--- Emitted when an entry is ousted because it exeed the `maximum_entries`.
-- @signal entry::ousted
-- @param o The object.

--- Emitted when an object is pushed to the history.
--
-- It is sent for both new and existing objects.
--
-- @signal entry::pushed
-- @param o The object.
-- @see added

--- Emitted when an object is added to the history.
--
-- It is sent only for *new* entries that were not already in the stack.
--
-- @signal entry::added
-- @param o The object.
-- @see pushed

--- Emitted when an object is removed from the stack.
--
-- @signal entry::removed
-- @param o The object.

local it_methods = {}

local function init_current(self)
    if self._current then return self._current end

    self._current = self._delta > 0 and 1 or #self._history._private.entries
    return self._current
end

local function it_rotate(self)
    init_current(self)

    -- Don't allow the index to go out of bound.
    if not self.rotate then
        self._current = math.max(1, self._current)
        self._current = math.min(self._current, #self._history._private.entries)
        return
    end

    -- Rotate from the edges
    if self._current > #self._history._private.entries then
        self._current = 1
    elseif self._current < 1 then
        self._current = #self._history._private.entries
    end
end

function it_methods:reset()
    self._current = nil
end

function it_methods:next()
    assert(self._history.paused, "Iterators are only available on paused history objects")
    init_current(self)
    self._current = self._current + self._delta
    it_rotate(self)
end

function it_methods:previous()
    assert(self._history.paused, "Iterators are only available on paused history objects")
    init_current(self)
    self._current = self._current - self._delta
    it_rotate(self)
end

function it_methods:select()
    assert(self._history.paused, "Iterators are only available on paused history objects")

    self._history:emit_signal(
        "request::select",
        self._history._private.entries[init_current(self)]
    )
end

-- Convenience wrapper intended to be passed to connect_signal.
function it_methods:select_next()
    self:next()
    self:select()
end

-- Convenience wrapper intended to be passed to connect_signal.
function it_methods:select_previous()
    self:previous()
    self:select()
end

local function remove(self, value, value2)
    -- Allow to be used as function or method
    if self == value then value = value2 end

    local idx = self._private.values[value]

    if idx then
        table.remove(self._private.entries, idx)

        -- Update the index
        for i = idx, #self._private.entries do
            local v = self._private.entries[i]
            self._private.values[v] = i
        end
    end

    self._private.counter[value] = nil

    -- Invalidate all iterators.
    for _, i in ipairs(self._private.iterators) do
        i:reset()
    end

    self:emit_signal("entry::removed", value)
end

local function push(self, value, value2)
    -- Remove all the boilerplate necessary to implement Windows/macOS Alt+Tab
    -- like behavior. While navigating the list, the, for example, clients
    -- should be focused but the history remain unchanged.
    if self.paused then return end

    -- Allow to be used as function or method
    if self == value then value = value2 end

    -- Don't allow direct duplicates, it probably happens because multiple
    -- signals are connected.
    if self._private.entries[#self._private.entries] == value then return end

    local m = self.mode

    if m == "multi" then
        table.insert(self._private.entries, value)
    else
        remove(self, value)

        table.insert(self._private.entries, value)

        if not self._private.values[value] then
            self:emit_signal("entry::added", value)
        end

        self._private.values[value] = #self._private.entries
    end

    if self.count then
        self._private.counter[value] = (self._private.counter[value] or 0) + 1
    end

    -- Remove enough entries to prevent going above the limite
    while #self._private.entries > (self.maximum_entries or math.huge) do
        local e = self._private.entries[1]
        remove(self, e)
        self:emit_signal("entry::ousted", e)
    end

    -- Invalidate all iterators.
    for _, i in ipairs(self._private.iterators) do
        i:reset()
    end

    self:emit_signal("entry::pushed", value)
end

--- Pop (remove) the most recent entry.
--
-- Note that this is a function, not a method. It is safe to use directly
-- in `connect_signal` or as a callback.
--
-- @method pop_newest

--- Pop (remove) the least recent entry.
--
-- Note that this is a function, not a method. It is safe to use directly
-- in `connect_signal` or as a callback.
--
-- @method pop_oldest

local function pop_newest(self)
    local v = self._private.entries[#self._private.entries]
    if not v then return end

    self._private.values[v] = nil
    table.remove(self._private.entries, #self._private.entries)
end

local function pop_oldest(self)
    local v = self._private.entries[#self._private.entries]
    if not v then return end

    self._private.values[v] = nil
    table.remove(self._private.entries, 1)

    -- Update the reverse mapping.
    for i = 1, #self._private.entries do
        self._private.values[self._private.entries[i]] = i
    end
end

-- Hack to make them available as functions *and* methods. It's useful when
-- used with `awful.keygrabber` or using:
--
--    capi.client.connect_signal("focus", myhistory.push_front)
--
-- It's unconventional, but on the other hand removes tons of boilerplate
-- `function() me:foo() end` style code. Given integration with existing modules
-- is a very important goal for this module, this is "a good idea".
--
local function add_functions(ret)
    gtable.crush(ret, {
        resume     = function(     ) ret.paused = false           end,
        pause      = function(     ) ret.paused = true            end,
        push       = function(...  ) return push       (ret, ...) end,
        pop_newest = function(value) return pop_oldest (ret     ) end,
        pop_oldest = function(value) return pop_newest (ret     ) end,
        remove     = function(...  ) return remove     (ret, ...) end,
    }, true)
end

-- Apply the filters and emit the signals.
local function iterate_common(self, delta)
    --TODO
end

-- It isn't "really" a `gears.object`, but mimics some aspects.
local function iterator_common(self, delta, args)

    local ret = {
        _delta   = delta,
        _history = self,
        _signals = {},
        rotate   = args.rotate == nil and true or args.rotate,
        filter   = args.filter,
    }

    -- Hacky trick to get the signals without yet another implementation.
    ret.connect_signal    = gobject.connect_signal
    ret.disconnect_signal = gobject.disconnect_signal
    ret.emit_signal       = gobject.emit_signal

    -- Make functions (instead of methods) for next/previous/select so they
    -- can directly be attached to `connect_signal`.
    for name, f in pairs(it_methods) do
        ret[name] = function() f(ret) end
    end

    table.insert(self._private.iterators, ret)

    return setmetatable(ret, {
        __call = function()
            -- Allow direct `for` loop iteration
            local cur = init_current(ret)
            ret._current = cur + delta
            return ret._history._private.entries[cur]
        end,
        __index = function(_, k)
            if k == "current" then
                return ret._history._private.entries[init_current(ret)]
            elseif k == "index" then
                return init_current(ret)
            end

            return it_methods[k]
        end,
        __newindex = function(_, k, v)
            if k == "index" then
                assert(v > 0 and v <= #ret._history._private.entries)
                init_current(ret)
                ret._current = v
            elseif k == "current" then
                -- This isn't very good when values are present multiple time,
                -- but otherwise ok.
                for idx, val in ipairs(ret._history._private.entries) do
                    if val == v then
                        init_current(ret)
                        ret._current = idx
                        return
                    end
                end
            else
                rawset(ret, k, v)
            end
        end
    })
end

function module:iterate_newest(args)
    return iterator_common(self, -1, args or {})
end

function module:iterate_oldest(args)
    return iterator_common(self, 1, args or {})
end

function module:oldest()
    return self._private.entries[1]
end

function module:newest()
    return self._private.entries[#self._private.entries]
end

function module:get_paused()
    return self._private.paused or false
end

function module:set_paused(v)
    if self._private.paused == v then return end
    self._private.paused = v
    if v then
        self:emit_signal("paused")
    else
        self:emit_signal("resumed")
    end
end

for _, prop in ipairs {"count", "maximum_entries"} do
    module["get_"..prop] = function(self) return self._private[prop] end

    module["set_"..prop] = function(self, value)
        self._private[prop] = value
        self:emit_signal("property::"..prop)
    end
end

--- Create a new history tracking object.
-- @tparam table args
-- @tparam[opt=false] boolean args.count Track the number of time an item is
--  pushed.
-- @return A new history tracking object.
-- @constructorfct gears.history

local function new(_, args)
    local ret = gobject {
        enable_auto_signals = true,
        enable_properties   = true,
    }

    rawset(ret, "_private", {
        entries   = {},
        values    = {},
        counter   = {},
        iterators = setmetatable({}, {__mode="v"}),
        count     = args.count and true or false,
    })

    ret.mode = "heap"

    gtable.crush(ret, module, true)

    add_functions(ret)

    gtable.crush(ret, args)

    return ret
end

--@DOC_object_COMMON@

--- Iterator methods.
-- Iterators objects are created using `:iterate_oldest` and `:iterate_newest`.
--
-- Note that that is a function rather than a method, it can be passed directly
-- to `connect_signal`.
--
-- @section iterator

--- Move the iterator to the next element.
--
-- Note that that is a function rather than a method, it can be passed directly
-- to `connect_signal`.
--
-- @method next
-- @see previous
-- @see select_next

--- Move the iterator to the previous element.
--
-- Note that that is a function rather than a method, it can be passed directly
-- to `connect_signal`.
--
-- @method previous
-- @see next
-- @see select_previous

--- Move the iterator to the next element and select it.
--
-- Note that that is a function rather than a method, it can be passed directly
-- to `connect_signal`.
--
-- @method select_next
-- @see previous

--- Move the iterator to the previous element and select it.
--
-- Note that that is a function rather than a method, it can be passed directly
-- to `connect_signal`.
--
-- @method select_previous
-- @see next

--- Emit the `request::select` signal on the current object.
--
-- Note that that is a function rather than a method, it can be passed directly
-- to `connect_signal`.
--
-- @method select

--- Reset the iterator position.
-- @method reset

--- Iterator properties.
-- @section iterator_properties

--- The current entry.
--
-- The type depends on the type of object being stored in the history.
--
-- @property current

--- The current index.
-- @property index
-- @param number

--- Start over when `:next()` or `:previous()` reach the end.
-- @property rotate
-- @param[opt=true] boolean

--- The filter function used to skip some entries.
--
-- **Clients from the current tag:**
--
--     function(c)
--         return (not client.focus) or (
--                 c.screen == client.focus.screen and c.first_tag.selected
--             )
--     end
--
-- **Clients from the current screen:**
--
--     function(c) return c.screen == awful.screen.focused() end
--
-- **Clients from the same application as the focused client:**
--
--    function(c) return (not client.focus) or client.focus.class == c.class end
--
-- **Tags from the current screen:**
--
--     function(t) return t.screen == awful.screen.focused() end
--
-- **Tags with clients in them:**
--
--    function(t) return #t:clients() > 0 end
--
-- @property filter
-- @param function

return setmetatable(module, {__call = new})
