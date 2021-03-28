--- Utility module to convert functions to Excel like objects.
--
-- When converting a function into a `gears.reactive` object, all
-- properties accessed by the function are monitored for changes.
--
-- When such a change is detected, the `property::value` signal is emitted. When
-- used within a widget declarative construct, the property it is attached
-- to will also be automatically updated.
--
-- Theory
-- ======
--
-- ![Client geometry](../images/gears_reactive.svg)
--
-- To use this module and, more importantly, to understand how to write something
-- that actually works based on it, some background is required. Most AwesomeWM
-- objects are based on the `gears.object` module or it's `C` equivalent. Those
-- objects have "properties". Behind the scene, they have getters, setters and
-- a signal to notify the value changed.
--
-- `gears.reactive` adds a firewall like sandbox around the function. It
-- intercept any `gears.object` instance trying to cross the sandbox boundary
-- and replace them with proxy objects. Those proxies have built-in
-- introspection code to detect how they are used. This is then converted into
-- a list of objects and signals to monitor. Once one of the monitored object
-- emits one of the monitored signal, then the whole function is re-evaluated.
-- Each time the function is evaluated, the "target" properties are updated. The
-- reactive function result or any call to external function from within goes
-- through the firewall again and any proxies are removed.
--
-- That design has one big limitation. It cannot detect any changes which are
-- not directly part of `gears.object` instance. You cannot use random tables
-- and expect the function to be called when it's content change. To work
-- around this, it is recommanded to make "hub" objects to store the data used
-- within the reactive function.
--
-- Recommanded usage
-- =================
--
-- The best way to use `gears.reactive` is when the value used within the
-- expressions are part of other objects. It can be a `gears.watcher`, but
-- it can also be a custom object:
--
-- @DOC_wibox_widget_declarative_reactive_EXAMPLE@
--
-- Limitations
-- ===========
--
-- `gears.reactive` is pushing Lua way beyond what it has been designed for.
-- Because of this, there is some limitations.
--
-- * This module will **NOT** try to track the change of other
--   functions and methods called by the expression. It is **NOT** recursive
--   and only the top level properties are tracked. This is a feature, not a
--   bug. If it was recursive, this list of limitations or gotchas would be
--   endless.
-- * This only works with `gears.object` and Core API objects which implement
--   the `property::*****` signals correctly. If it is a regular Lua table
--   or the property signals are incorrectly used, the value changes cannot
--   be detected. If you find something that should work, but doesn't in
--   one of the AwesomeWM API, [please report a bug](https://github.com/awesomeWM/awesome/issues/new).
-- * More generally, when making a custom `gears.object` with custom setters,
--   it is the programmer responsibility to emit the signal. It is also
--   required to only emit those signals when the property actually changes to
--   avoid an unecessary re-evaluations.
-- * Changing the type of the variables accessed by the reactive function
--   (its "upvalues") after the reactive expression has been created wont
--   be detected. It will cause missed updates and, potentially, hard to debug
--   Lua errors within the proxy code itself.
-- * Internally, the engine tries its best to prevent the internal proxy objects
--   to leak out the sandbox. However this cannot be perfect, at least not
--   without adding limitation elsewhere. It is probably worth reporting a bug if
--   you encounter such an issue. But set your expectations, not all corner case
--   can be fixed.
-- * Don't use rawset in the expression.
-- * If the return value is a table, only the first 3 nesting levels are sanitized.
--   Avoid using nested tables in the returned value if you can. `gears.object`
--   instances *should* be fine.
-- * There is currently no way to disable a reactive expression once it's
--   been defined. This may change eventually.
-- * Rio Lua 5.1 (not LuaJIT 2.1) is currently not support. If you need it,
--   [please report a bug](https://github.com/awesomeWM/awesome/issues/new).
--
-- @author Emmanuel Lepage-Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2017-2020 Emmanuel Lepage-Vallee
-- @classmod gears.reactive

-- This file is provided under the BSD 2 clause license since it is better than
-- any existing implementation (most of which only support Lua 5.1).
--
-- Copyright 2020 Emmanuel Lepage-Vallee &lt;elv1313@gmail.com&gt;
--
-- Redistribution and use in source and binary forms, with or without modification,
-- are permitted provided that the following conditions are met:
--
-- 1. Redistributions of source code must retain the above copyright notice, this
--    list of conditions and the following disclaimer.
--
-- 2. Redistributions in binary form must reproduce the above copyright notice,
--    this list of conditions and the following disclaimer in the documentation
--    and/or other materials provided with the distribution.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
-- DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
-- FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
-- (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
-- OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
-- THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
-- NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
-- IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

local gobject = require("gears.object")
local gtable = require("gears.table")
local gtimer = require("gears.timer")
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

-- It's only possible to know when something changes if it has signals.
local function is_gears_object(input)
    return type(input) == "table" and rawget(input, "emit_signal")
end

-- If the sandbox proxies were to leave the reactive expressions, bad
-- things would happen. Catch functions and constructors to make sure
-- we strip away the proxy.
local function is_callable(object)
    local t = type(object)

    if t == "function" then return true  end

    if not (t == "table" or t == "userdata") then return false end

    local mt = getmetatable(object)

    return mt and mt.__call ~= nil
end

-- Get the upvalue current value or cached value.
local function get_upvalue(proxy_md)
    if type(proxy_md) ~= "table" then return proxy_md end

    return proxy_md._reactive.getter and
        proxy_md._reactive.getter() or proxy_md._reactive.input
end

-- Attempt to remove the proxy references. There might be loops
-- and stackoverflows, so it will only go 3 level deep. With some
-- luck, the result wont be a table. Even then, hopefully it wont
-- contain "raw" sub-tables. If it contains `gears.object`, it's
-- safe since they will be proxied.
local sanitize_return = nil
sanitize_return = function(input, depth)
    if (depth or 0) > 3 then return input end

    -- The first `input` is always a table, so it is safe to call `pairs`.
    for k, v in pairs(input) do
        local t = type(v)

        if t == "table" then
            if v._reactive then
                -- No need to go deeper unless the user used rawset.
                input[k] = get_upvalue(v)
            else
                -- It's an unproxied raw table, dig further.
                sanitize_return(v, (depth or 1) + 1)
            end
        end
    end

    return input
end

-- The actual code that connects the gears.object signals.
local function add_key(self, key)
    if type(key) ~= "string" or not is_gears_object(get_upvalue(self)) then return end

    local input, objs = get_upvalue(self), self._reactive.origin.objects

    objs[input] = objs[input] or {}

    if objs[input][key] then return end

    objs[input][key] = true

    -- Prefer the `weak` version to avoid memory leaks. If nothing is holding a
    -- reference, then the value wont change. Yes, there is some hairy corner cases.
    -- Yes, the expression proxy itself is holding a strong reference so this wont
    -- get used very often.
    if input.weak_connect_signal then
        input:weak_connect_signal("property::"..key, self._reactive.origin.callback)
    else
        input:connect_signal("property::"..key, self._reactive.origin.callback)
    end
end

local function gen_proxy_md(input, origin, getter)
    assert(input and origin)
    return {
        input      = (not getter) and input or nil,
        getter     = getter,
        children   = {},
        properties = {},
        origin     = origin
    }
end

local create_proxy = nil

-- Un-proxy the function input and proxy its output.
local function proxy_call(self, ...)
    local newargs, newrets = {}, {}

    -- Remove the proxies before calling the function.
    for _, v in ipairs{...} do
        if type(v) == "table" and v._reactive then
            v = get_upvalue(v)
        end
        table.insert(newargs, v)
    end

    local rets

    if #newargs > 0 then
        rets = {get_upvalue(self)(unpack(newargs))}
    else
        rets = {get_upvalue(self)(nil)}
    end

    -- Add new proxies to make sure changes are detected if something
    -- like `a_function().foo` is used. Since we still have legacy accessor
    -- implemented as method (t:clients(), c:tags(), etc), this is actually
    -- likely to happen.
    for _, v in ipairs(rets) do
        local ret, _ = create_proxy(v, self._reactive.origin)
        table.insert(newrets, ret)
    end

    return unpack(newrets)
end

-- Build a tree of proxies or return the primitives.
local function proxy_index(self, key)
    local up = get_upvalue(self)

    -- Since it would otherwise print a cryptic error.
    assert(
        type(up) == "table",
        "Trying to index a "..type(up).." value in a `gears.reactive` expression."
    )

    local upk = up[key]

    -- Connect.
    add_key(self, key)

    local ret, is_proxy = create_proxy(upk, self._reactive.origin)

    -- Always query the non-proxy upvalue. We cannot detect if they
    -- change.
    if is_proxy then
        rawset(self, key, ret)
    end

    return ret
end

-- Set valuers trough the proxy.
local function proxy_newindex(self, key, value)
    rawset(self, key, create_proxy(value, self._reactive.origin))

    -- Strip the proxy before setting the value on the original object.
    if type(value) == "table" and value._reactive then
        value = get_upvalue(value)
    end

    local v = get_upvalue(self)

    v[key] = value

    -- Connect.
    if is_gears_object(v) and not self._reactive.origin.objects[self][key] then
        add_key(self, key)
    end
end

-- It's possible that multiple proxy points to the same value.
-- While we could make a large map of all proxies, it's simpler
-- to just implement __eq/__lt/__le and be done.
local function proxy_equal(a, b)
    a, b = get_upvalue(a), get_upvalue(b)

    return a == b
end

local function proxy_lesser(a, b)
    a, b = get_upvalue(a), get_upvalue(b)

    return a < b
end

local function proxy_lesser_equal(a, b)
    a, b = get_upvalue(a), get_upvalue(b)

    return a <= b
end

local function proxy_tostring(o)
    return tostring(get_upvalue(o))
end

-- Wrap tables and functions into a proxy object.
create_proxy = function(input, origin, getter)
    local t = type(input)
    local is_call = is_callable(input)

    -- Everything but the tables are immutable.
    if t ~= "table" and not is_call then return input, false end

    -- Remove any foreign proxy.
    if t ~= "function" and input._reactive and input._reactive.origin ~= origin then
        input = get_upvalue(input)
    end

    return setmetatable({_reactive = gen_proxy_md(input, origin, getter)}, {
        __index    = proxy_index,
        __newindex = proxy_newindex,
        __call     = proxy_call,
        __eq       = proxy_equal,
        __lt       = proxy_lesser,
        __le       = proxy_lesser_equal,
        __tostring = proxy_tostring
    }), true
end

-- Create a "fake" upvalue ref because otherwise the sandbox could
-- "leak" and set upvalue in actual code outside of the function.
-- By "leak", I mean the proxy would become visible from outside
-- of the sandbox by the reactive expression "siblings" sharing the
-- same environment.
local function copy_upvalue_reference(loaded, idx, value)
    -- Make sure it has a fresh upvalue reference. Something that
    -- we are sure cannot be shared with something else.
    local function fake_upvalue_env()
        return value
    end

    debug.upvaluejoin(loaded, idx, fake_upvalue_env, 1) --luacheck: ignore
end

-- Create a meta-getter for the original `fct` upvalue at index `idx`.
-- This will allow all the other code to dynamically get a "current" version
-- of the upvalue rather than something from the cache. It will also avoid
-- having to store a strong reference to the original value.
local function create_upvalue_getter(fct, idx)
    local placeholder = nil

    local function fake_upvalue_getter()
        return placeholder
    end

    -- Using this, each time `fake_upvalue_getter` is called, it
    -- will return the current upvalue. This means we don't have to
    -- cache it. Thus, the cache cannot get outdated. The drawback if
    -- that if the type changes from an object to a primitive, it
    -- will explode.
    debug.upvaluejoin(fake_upvalue_getter, 1, fct, idx) --luacheck: ignore

    return fake_upvalue_getter
end

-- Create a copy of the function and replace it's environment.
local function sandbox(fct, env, vars, values)
    -- We must serialize the function for several reasons. First of all, it
    -- might get wrapped into multiple `gears.reactive` objects (which we should
    -- handle differently, but currently allow) or share the "upvalue environemnt"
    -- (this variables from the upper execution context and stack frames).
    --
    -- For example, if 2 function both access the variables "foo" and "bar"
    -- from the global context, they might end up with the same execution
    -- environment. If we didn't create a copy, calling `debug.upvaluejoin`
    -- woulc affect both functions.
    local dump   = string.dump(fct)
    local loaded = load(dump, nil, nil, env)

    -- It doesn't seem possible to "just remove" the upvalues. It's not possible
    -- to have the catch-all in the metatable. It would have been nicer since this
    -- code is redundant with the metatable (which are still needed for _G and _ENV
    -- content).
    for name, k in pairs(vars) do
        if is_callable(values[name]) or type(values[name]) == "table" then
            -- For table, functions and objects, use a proxy upvalue.
            copy_upvalue_reference(
                loaded,
                k,
                create_proxy(values[name], env.__origin, create_upvalue_getter(fct ,k))
            )
        else
            -- For string, booleans, numbers and function, use the real upvalue.
            -- This means if it is changed by something else, the sandboxed
            -- copy sees the change.
            debug.upvaluejoin(loaded, k, fct, k) --luacheck: ignore
        end
    end

    return loaded
end

-- `getfenv` and `setfenv` would simplify this a lot, but are not
-- present in newer versions of Lua. Technically, we could have 3
-- implementation optimized for each Lua versions, but this one
-- seems to be portable (until now...). So while the performance
-- isn't as good as it could be, it's maintainable.
local function getfenv_compat(fct, root)
    local vars, soft_env = {}, {}

    local origin = {
        objects = {},
        callback = function()
            root:emit_signal("property::value")
        end
    }

    for i = 1, math.huge do
        local name, val = debug.getupvalue(fct, i)

        if name == nil then break end

        soft_env[name] = val

        if not vars[name] then
            vars[name] = i
        end
    end

    -- Create the sandbox.
    local self = {__origin = origin}
    local sandboxed = sandbox(fct, self, vars, soft_env)

    return setmetatable(self, {
        __index = function(_, key)
            if _G[key] then
                return create_proxy(_G[key], origin)
            end

            return soft_env[key]
        end,
        __newindex = function(_, key, value)
            -- This `if` might be dead code.
            if vars[key] then
                debug.setupvalue(sandboxed, vars[key], value)

                -- Do not try to disconnect the old one. It would make the code too complex.
                if (not soft_env[key]) or get_upvalue(soft_env[key]) ~= value then
                    soft_env[key] = create_proxy(value, origin)
                end
            else
                rawset(vars, key, create_proxy(value, origin))
            end
        end,
        __call = function()
            return unpack(sanitize_return({sandboxed()}))
        end
    })
end

local module = {}

local function real_set_value(self, force)
    if self._private.delayed_started and not force then return end

    local function value_transaction()
        -- If `get_value` was called, then this transaction is no longer
        -- pending.
        if not self._private.delayed_started then return end

        -- Reset the delay in case the setter causes the expression to
        -- change.
        self._private.delayed_started = false

        self._private.value = self._private.origin()
        self._private.evaluated = true

        for _, target in pairs(self._private.targets) do
            local obj, property = target[1], target[2]
            obj[property] = self._private.value
        end
    end

    if self._private.delayed and not force then
        gtimer.delayed_call(value_transaction)
        self._private.delayed_started = true
    else
        self._private.delayed_started = true
        value_transaction()
    end
end

--- A function which will be evaluated each time its content changes.
--
-- @property expression
-- @param function
-- @propemits true false

--- The current value of the expression.
-- @property value
-- @propemits false false

--- Only evaluate once per event loop iteration.
--
-- In most case this is a simple performance win, but there is some
-- case where you might want the expression to be evaluated each time
-- one of the upvalue "really" change rather than batch them. This
-- option is enabled by default.
--
-- @property delayed
-- @tparam[opt=true] boolean delayed
-- @propemits true false

function module:get_delayed()
    return self._private.delayed
end

function module:set_delayed(value)
    if value == self._private.delayed then return end

    self._private.delayed = value
    self:emit_signal("property::delayed", value)
end

function module:set_expression(value)
    self:disconnect()
    self._private.origin = getfenv_compat(value, self)

    self:connect_signal("property::value", real_set_value)
end

function module:get_value()
    -- This will call `real_set_value`.
    if (not self._private.evaluated) and self._private.origin then
        self:refresh()
    end

    if self._private.delayed_started then
        real_set_value(self, true)
    end

    return self._private.value
end

function module:set_value()
    assert(false, "A value cannot be set on a `gears.reactive` instance.")
end

--- Disconnect all expression signals.
--
-- @method disconnect

function module:disconnect()
    if self._private.origin then
        for obj, properties in pairs(self._private.origin.__origin.objects) do
            for property in pairs(properties) do
                obj:disconnect_signal("property::"..property, self._private.origin.__origin.callback)
            end
        end
    end
end

--- Recompute the expression.
--
-- When the expression uses a non-object upvalue, the changes cannot
-- be auto-retected. Calling `:refresh()` will immediatly recompute the
-- expression.
--
-- @method refresh

function module:refresh()
    if self._private.origin then
        self:emit_signal("property::value")
    end
end

-- Add a new target property and object.
function module:_add_target(object, property)
    local hash = tostring(object)..property
    if self._private.targets[hash] then return end

    self._private.targets[hash] = {object, property}

    self:emit_signal("target_added", object, property)

    if self._private.evaluated then
        object[property] = self._private.value
    else
        real_set_value(self)
    end
end

--- Emitted when a new property is attached to this reactive expression.
--
-- @signal target_added
-- @tparam gears.object object The object (often the widget).
-- @tparam string property The property name.

function module:_set_declarative_handler(parent, key)
    -- Lua "properties", aka table.foo must be strings.
    assert(
        type(key) == "string",
        "gears.reactive can only be used ob object properties"
    )

    self:_add_target(parent, key)
end

--- Create a new `gears.reactive` object.
-- @constructorfct gears.reactive
-- @tparam table args
-- @tparam function args.expression A function which accesses other `gears.object`.
-- @tparam gears.object args.object Any AwesomeWM object.
-- @tparam string args.property The name of the property to track.

local function new(_, args)
    -- It *can* be done. Actually, it is much easier to implement this module
    -- on 5.1 since `setfenv` is much simpler than `debug.upvaluejoin`. However,
    -- unless someone asks, then why support 2 incompatible code paths. Luajit 2.1
    -- supports `debug.upvaluejoin`.
    assert(
        debug.upvaluejoin,  --luacheck: ignore
        "Sorry, `gears.reactive` doesn't support Lua 5.1 at this time"
    )

    if type(args) == "function" then
        args = {expression = args}
    end

    local self = gobject {
        enable_properties = true,
    }

    rawset(self, "_private", {
        targets = {},
        delayed = true
    })

    gtable.crush(self, module, true)

    gtable.crush(self, args, false)

    self:connect_signal("property::value", real_set_value)

    return self
end

--@DOC_object_COMMON@

return setmetatable(module, {__call = new})
