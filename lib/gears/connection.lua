--- Object oriented way to connect objects.
--
-- All AwesomeWM objects have a `emit_signal` method.
-- They allow to attach business logic to a property value change or random
-- types of events.
--
-- The default way to attach business logic to signals is to use `connect_signal`.
-- It allows to call a function when the signal is emitted. This remains the most
-- common way to perform a connection. However, it is very verbose to use that
-- construct alongside the declarative widget system. `gears.connection` is much
-- easier to integrate in such constructs:
--
-- @DOC_wibox_decl_doc_connection4_EXAMPLE@
--
-- Limitations
-- ===========
--
-- * When used directly as a value to a declarative object
--   (`text = gears.connection{...}`), it is necessary to manually disconnect
--   the connectio if you want it to stop being auto-updated.
--
-- @author Emmanuel Lepage-Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2019-2020 Emmanuel Lepage-Vallee
-- @classmod gears.connection

local gobject = require("gears.object")
local gtable = require("gears.table")
local gtimer = require("gears.timer")

local module = {}

local function get_class(name)
    if type(name) ~= "string" then return name end

    return _G[name] or require(name)
end

local function gen_get_children_by_id(self)
    return function(id)
        return (self._private.env_ids or {})[id] or {}
    end
end

local function get_env(_, cb)
    local env, position = nil, nil

    for i = 1, math.huge do
        local name, val = debug.getupvalue(cb, i)

        if name == "_ENV" then
            env, position = val, i
            break
        elseif name == nil then
            break
        end
    end

    return env, position
end

-- Expose the declarative tree `id`s to the callback as variables.
--
-- Note that this runs into a slight conflict with the ability to
-- place a widget in multiple trees. If it happens, then obviously
-- there will be `id` conflicts. That's solvable by only using
-- `get_children_by_id` in such situation. As of now, there is no
-- code to track `id`s across trees. So the callback will only have the
-- ids from the tree it was last added to.
local function extend_env(self)
    local cb, ids = self._private.callbacks[1], self._private.env_ids

    if (not cb) or (not ids) then return end

    self._private.env_init = true

    local env, position = get_env(self, cb)

    if not env then return end

    local gcbi = nil

    local new_env = setmetatable({}, {
        __index = function(_, key)
            if key == "get_children_by_id" then
                gcbi = gcbi or gen_get_children_by_id(self)
                return gcbi
            elseif ids[key] and #ids[key] == 1 then
                return ids[key][1]
            end

            local v = env[key]

            if v then return v end

            return _G[key]
        end,
        __newindex = function(_, key, value)
            _G[key] = value
        end
    })

    debug.setupvalue(cb, position, new_env)
end

local function set_target(self)
    local p = self._private

    local has_target = p.target
        and p.initiate ~= false

    local has_source = #p.sources >= 1
        and #p.source_properties >= 1

    extend_env(self)

    for _, callback in ipairs(self._private.callbacks) do
        local ret = callback(
            p.sources[1],
            p.target,
            (p.sources[1] and p.source_properties[1]) and
                p.sources[1][p.source_properties[1]] or nil
        )

        if self.target_property and self._private.target then
            self._private.target[self.target_property] = ret
        end
    end

    if p.target_method then
        p.target[p.target_method]()
    end

    -- There isn't enough information to initiate anything yet.
    if not (has_target and has_source) then return end

    if p.target_property then
        p.target[p.target_property] = p.sources[1][p.source_properties[1]]
    end
end

-- When all properties necessary to set the initial value are set.
local function initiate(self)
    if self._private.initiating then return end

    -- We don't know if properties will be overriden or if a callback/method
    -- will be added. Better wait.
    gtimer.delayed_call(function()
        -- It might have been disabled since then.
        if not self._private.enabled then return end

        set_target(self)
        self._private.initiating = false
    end)

    self._private.initiating = true
end

function module:get_initiate()
    return self._private.initiate
end

--- If the property should be set when the target object is defined.
--
-- It is **enabled** by default for convinience.
--
-- @DOC_text_gears_connection_initiate_EXAMPLE@
--
-- @property initiate
-- @tparam[opt=true] boolean string initiate
-- @propemits true false

function module:set_initiate(value)
    if self._private.initiate == value then return end
    self._private.initiate = value
    self:emit_signal("property::initiate", value)

    if value then
        initiate(self)
    end
end

--- Turn this connection on or off.
--
-- @DOC_text_gears_connection_enabled_EXAMPLE@
--
-- @property enabled
-- @tparam boolean enabled
-- @see disconnect
-- @see reconnect
-- @propemits true false

function module:get_enabled()
    return self._private.enabled
end

function module:set_enabled(value)
    if value == self._private.enabled then return end

    self._private.enabled = value
    self:emit_signal("property::enabled", value)
end

--- A list of source object signals.
--
-- @property signals
-- @tparam table signals
-- @propemits true false
-- @see signal
-- @see source_property


function module:get_signals()
    return self._private.signals
end

function module:set_signals(value)
    self:disconnect()
    self._private.signals = value
    self:reconnect()

    self:emit_signal("property::signal", value[1])
    self:emit_signal("property::signals", value)

    initiate(self)
end

function module:get_signal()
    return self._private.signals < 2 and self._private.signals[1] or nil
end

--- The (source) signal to monitor.
--
-- Note that `signals` and `source_property` are also provided to simplify
-- common use cases.
--
-- @property signal
-- @param string
-- @propemits true false
-- @see signals
-- @see source_property

function module:set_signal(signal)
    self._private.signals = {signal}
end

--- The object for the right side object of the connection.
--
-- When used in a widget declarative tree, this is implicit and
-- is the parent object.
--
-- @DOC_text_gears_connection_target_EXAMPLE@
--
-- @property target
-- @tparam gears.object target
-- @propemits true false
-- @see target_property
-- @see target_method

function module:get_target()
    return self._private.target
end

function module:set_target(target)
    self._private.target = target
    self:emit_signal("property::target", target)
    initiate(self)
end

--- The target object property to set when the source property changes.
--
-- @property target_property
-- @tparam string target_property
-- @propemits true false
-- @see target
-- @see target_method
-- @see source_property

function module:get_target_property()
    return self._private.target_property
end

function module:set_target_property(value)
    self._private.target_property = value
    self:emit_signal("property::target_property", value)

    initiate(self)
end

--- Rather than use a property, call a method.
--
-- @DOC_text_gears_connection_method_EXAMPLE@
--
-- @property target_method
-- @tparam string target_method
-- @propemits true false
-- @see target
-- @see target_property

function module:get_target_method()
    return self._private.target_method
end

function module:set_target_method(value)
    self._private.target_method = value
    self:emit_signal("property::target_method", value)

    initiate(self)
end

--- Use a whole class rather than an object as source.
--
-- Many classes, like `client`, `tag`, `screen` and `naughty.notification`
-- provide class level signals. When any instance of those classes emit a
-- signal, it is forwarded to the class level signals.
--
-- @DOC_text_gears_connection_class_EXAMPLE@
--
-- @property source_class
-- @tparam class|string source_class
-- @propemits true false
-- @see source
-- @see source_property

function module:set_source_class(class)
    self:disconnect()
    self._private.source_class = get_class(class)
    self:reconnect()
    self:emit_signal("property::source_class", self._private.source_class)
end

function module:get_source_class()
    return self._private.source_class
end

--- The source object (connection left hand side).
-- @property source
-- @tparam gears.object source
-- @propemits true false
-- @see sources
-- @see source_class

function module:get_source()
    return self._private.sources[1]
end

function module:set_source(source)

    self:disconnect()
    self._private.sources = {source}
    self:reconnect()

    self:emit_signal("property::source", source)
    self:emit_signal("property::sources", self._private.sources)

    initiate(self)
end

--- The source object(s)/class property.
--
-- @property source_property
-- @tparam string source_property
-- @propemits true false

function module:get_source_property()
    return #self._private.source_properties == 1 and
        self._private.source_properties[1] or nil
end

function module:set_source_property(prop)
    self.source_properties = {prop}
end

function module:get_source_properties()
    return self._private.source_properties
end

function module:set_source_properties(props)
    self:disconnect()
    self._private.source_properties = props

    local signals = {}

    self:reconnect()

    for _, prop in ipairs(props) do
        table.insert(signals, "property::"..prop)
    end

    self.signals = signals

    self:emit_signal("property::source_property", props[1])
    self:emit_signal("property::source_properties", props)

end


--- A list of source objects (connection left hand side).
--
-- If many objects have the same signal, it's not necessary
-- to make multiple `gears.connection`. They can share the same.
--
-- @property sources
-- @tparam gears.object sources
-- @propemits true false
-- @see append_source_object
-- @see remove_source_object

function module:get_sources()
    return self._private.sources
end

function module:set_sources(sources)
    if not sources then
        sources = {}
    end

    self:disconnect()
    self._private.sources = sources
    self:reconnect()

    self:emit_signal("property::source", sources[1])
    self:emit_signal("property::sources", sources)

    initiate(self)
end

--- Add a source object.
--
-- @DOC_text_gears_connection_add_remove_EXAMPLE@
--
-- @method append_source_object
-- @tparam gears.object obj The object.
-- @see sources
-- @see remove_source_object
-- @see has_source_object
-- @see source
-- @see sources

function module:append_source_object(obj)
    if self:has_source_object(obj) then return end

    table.insert(self._private.sources, obj)
    self:emit_signal("property::sources", self._private.sources)

    if #self._private.sources == 1 then
        initiate(self)
    end
end

--- Remove a source object.
--
-- @method remove_source_object
-- @tparam gears.object obj The object.
-- @see sources
-- @see append_source_object
-- @see has_source_object
-- @see source
-- @see sources

function module:remove_source_object(obj)
    for k, o in ipairs(self._private.sources) do
        if obj == o then
            table.remove(self._private.sources, k)
            self:emit_signal("property::sources", self._private.sources)
            return true
        end
    end

    return false
end

--- Return true when `obj` is already a source object.
--
-- @method module:has_source_object
-- @tparam gears.object obj The object.
-- @see append_source_object
-- @see remove_source_object
-- @see source
-- @see sources

function module:has_source_object(obj)
    for _, o in ipairs(self._private.sources) do
        if o == obj then return true end
    end

    return false
end

--- A function called when the source changes.
--
--
-- @DOC_wibox_decl_doc_connection2_EXAMPLE@
--
-- The callback arguments are:
--
--     callback = function(source, target, sig_arg1, ...)
--                          /\     /\        /\     /\
--                           |      |         |      |
--              The  client -|      |         |      |
--           It will be the widget -|         |      |
--         Signal first argument, the client -|      |
--                       All other signal arguments -|
--
-- @property callback
-- @tparam function callback
-- @propemits true false

function module:get_callback()
    return self._private.callbacks[1]
end

function module:set_callback(cb)
    self._private.callbacks = {cb}

    self:emit_signal("property::callback", cb)

    self._private.env_init = false

    initiate(self)
end

-- When used in a declarative tree, this will be the
-- object it is initiated from. The `key` can be a number,
-- in which case we do nothing. It can also be a string,
-- in which case it becomes `target_property`
function module:_set_declarative_handler(parent, key, ids)
    self.target = parent

    self._private.env_ids = ids
    self._private.env_init = false

    if type(key) == "string" then
        self.target_property = key
    end

    initiate(self)
end

--- Disconnect this object.
--
-- @method disconnect
-- @see reconnect

function module:disconnect()
    if self._private.source_class then
        for _, sig in ipairs(self._private.signals) do
            self._private.source_class.disconnect_signal(
                sig, self._private._callback
            )
        end
    end

    for _, src in ipairs(self._private.sources) do
        for _, sig in ipairs(self._private.signals) do
            src:disconnect_signal(sig, self._private._callback)
        end
    end
end

--- Reconnect this object.
--
-- @method reconnect
-- @see disconnect

function module:reconnect()
    self:disconnect()

    if self._private.source_class then
        for _, sig in ipairs(self._private.signals) do
            self._private.source_class.connect_signal(
                sig, self._private._callback
            )
        end
    end

    for _, src in ipairs(self._private.sources) do
        for _, sig in ipairs(self._private.signals) do
            src:connect_signal(sig, self._private._callback)
        end
    end
end

--- Create a new `gears.connection` object.
--
-- @constructorfct gears.connection
-- @tparam table args
-- @tparam boolean args.initiate If the property should be set when the target object is defined.
-- @tparam boolean args.enabled Turn this connection on or off.
-- @tparam boolean args.signals A list of source object signals.
-- @tparam string args.signal The (source) signal to monitor.
-- @tparam gears.object args.target The object for the right side object of the connection.
-- @tparam string args.target_property The target object property to set when the source property changes.
-- @tparam string args.target_method Rather than use a property, call a method.
-- @tparam class|string args.source_class Use a whole class rather than an object as source.
-- @tparam gears.object args.source The source object (connection left hand side).
-- @tparam string args.source_property The source object(s)/class property.
-- @tparam gears.object args.sources A list of source objects (connection left hand side).
-- @tparam function args.callback A function called when the source changes.

local function new(_, args)
    local self = gobject {
        enable_properties = true,
    }

    rawset(self, "_private", {
        enabled           = true,
        signals           = {},
        initiate          = true,
        sources           = {},
        callbacks         = {},
        source_properties = {},
        target            = nil,
        _callback         = function()
            if not self._private.enabled then return end

            set_target(self)
        end
    })

    gtable.crush(self, module, true )
    gtable.crush(self, args  , false)

    return self
end

--@DOC_object_COMMON@

return setmetatable(module, {__call = new})
