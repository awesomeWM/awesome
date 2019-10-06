---------------------------------------------------------------------------
--- Rules for clients.
--
-- This module applies @{rules} to clients during startup (via @{client.manage},
-- but its functions can be used for client matching in general.
--
-- All existing `client` properties can be used in rules. It is also possible
-- to add random properties that will be later accessible as `c.property_name`
-- (where `c` is a valid client object)
--
-- Syntax
-- ===
-- You should fill this table with your rule and properties to apply.
-- For example, if you want to set xterm maximized at startup, you can add:
--
-- @DOC_sequences_client_rules_maximized_EXAMPLE@
--
-- If you want to set mplayer floating at startup, you can add:
--
-- @DOC_sequences_client_rules_floating_EXAMPLE@
--
-- If you want to put Firefox on a specific tag at startup. It is possible to
-- specify the tag with it's object or by name:
--
-- @DOC_sequences_client_rules_tags_EXAMPLE@
--
-- If you want to put Thunderbird on a specific screen at startup, use:
--
-- @DOC_sequences_client_rules_screens_EXAMPLE@
--
-- If you want to put Emacs on a specific tag at startup, and immediately switch
-- to that tag you can add:
--
-- @DOC_sequences_client_rules_switch_to_tags_EXAMPLE@
--
-- If you want to apply a custom callback to execute when a rule matched,
-- for example to pause playing music from mpd when you start dosbox, you
-- can add:
--
--     { rule = { class = "dosbox" },
--       callback = function(c)
--          awful.spawn('mpc pause')
--       end }
--
-- Note that all "rule" entries need to match. If any of the entry does not
-- match, the rule won't be applied.
--
-- If a client matches multiple rules, they are applied in the order they are
-- put in this global rules table. If the value of a rule is a string, then the
-- match function is used to determine if the client matches the rule.
--
-- If the value of a property is a function, that function gets called and
-- function's return value is used for the property.
--
-- To match multiple clients to a rule one need to use slightly different
-- syntax:
--
--     { rule_any = { class = { "MPlayer", "Nitrogen" }, instance = { "xterm" } },
--       properties = { floating = true } }
--
-- To match multiple clients with an exception one can couple `rules.except` or
-- `rules.except_any` with the rules:
--
--     { rule = { class = "Firefox" },
--       except = { instance = "Navigator" },
--       properties = {floating = true},
--     },
--
--     { rule_any = { class = { "Pidgin", "Xchat" } },
--       except_any = { role = { "conversation" } },
--       properties = { tag = "1" }
--     }
--
--     { rule = {},
--       except_any = { class = { "Firefox", "Vim" } },
--       properties = { floating = true }
--     }
--
-- Note that all rules can have an `id` field. This can then be used to find
-- the rule. For example, it can be used in `remove_rule` instead of the table.
--
-- Applicable client properties
-- ===
--
-- The table below holds the list of default client properties along with
-- some extra properties that are specific to the rules. Note that any property
-- can be set in the rules and interpreted by user provided code. This table
-- only represent those offered by default.
--
--@DOC_client_rules_index_COMMON@
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @ruleslib ruled.client
---------------------------------------------------------------------------

-- Grab environment we need

local capi = {client = client, awesome = awesome, screen = screen, tag = tag}
local table = table
local type = type
local ipairs = ipairs
local pairs = pairs
local atag = require("awful.tag")
local gobject = require("gears.object")
local gtable = require("gears.table")
local a_place = require("awful.placement")
local protected_call = require("gears.protected_call")
local aspawn = require("awful.spawn")
local gdebug = require("gears.debug")
local gmatcher = require("gears.matcher")
local amouse = require("awful.mouse")
local akeyboard = require("awful.keyboard")
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local module = {}

local crules = gmatcher()

--- Check if a client matches a rule.
-- @client c The client.
-- @tab rule The rule to check.
-- @treturn bool True if it matches, false otherwise.
-- @staticfct ruled.client.match
function module.match(c, rule)
    return crules:_match(c, rule)
end

--- Check if a client matches any part of a rule.
-- @client c The client.
-- @tab rule The rule to check.
-- @treturn bool True if at least one rule is matched, false otherwise.
-- @staticfct ruled.client.match_any
function module.match_any(c, rule)
    return crules:_match_any(c, rule)
end

--- Does a given rule entry match a client?
-- @client c The client.
-- @tab entry Rule entry (with keys `rule`, `rule_any`, `except` and/or
--   `except_any`).
-- @treturn bool
-- @staticfct ruled.client.matches
function module.matches(c, entry)
    return crules:matches_rule(c, entry)
end

--- Get list of matching rules for a client.
-- @client c The client.
-- @tab _rules The rules to check. List with "rule", "rule_any", "except" and
--   "except_any" keys.
-- @treturn table The list of matched rules.
-- @staticfct ruled.client.matching_rules
function module.matching_rules(c, _rules)
    return crules:matching_rules(c, _rules)
end

--- Check if a client matches a given set of rules.
-- @client c The client.
-- @tab _rules The rules to check. List of tables with `rule`, `rule_any`,
--   `except` and `except_any` keys.
-- @treturn bool True if at least one rule is matched, false otherwise.
-- @staticfct ruled.client.matches_list
function module.matches_list(c, _rules)
    return crules:matches_rules(c, _rules)
end

--- Remove a source.
-- @tparam string name The source name.
-- @treturn boolean If the source was removed.
-- @staticfct ruled.client.remove_rule_source
function module.remove_rule_source(name)
    return crules:remove_matching_source(name)
end

--- Apply ruled.client.rules to a client.
-- @client c The client.
-- @staticfct ruled.client.apply
function module.apply(c)
    return crules:apply(c)
end

--- Add a new rule to the default set.
--
-- @tparam table rule A valid rule.
function module.append_rule(rule)
    crules:append_rule("awful.rules", rule)
end

--- Add a new rules to the default set.
-- @tparam table rules A table with rules.
function module.append_rules(rules)
    crules:append_rules("awful.rules", rules)
end

--- Remove a new rule to the default set.
-- @tparam table|string rule A valid rule or a name passed in the `id` value
--  when calling `append_rule`.
function module.remove_rule(rule)
    crules:remove_rule("awful.rules", rule)
end

--- Add a new rule source.
--
-- A rule source is a provider called when a client is managed (started). It
-- allows to configure the client by providing properties that should be applied.
-- By default, Awesome provides 2 sources:
--
-- * `awful.rules`: A declarative matcher
-- * `awful.spawn`: Launch clients with pre-defined properties
--
-- It is possible to register new callbacks to modify the properties table
-- before it is applied. Each provider is executed sequentially and modifies the
-- same table. If the first provider set a property, then the second can
-- override it, then the third, etc. Once the providers are exhausted, the
-- properties are applied on the client.
--
-- It is important to note that properties themselves have their own
-- dependencies. For example, a `tag` property implies a `screen`. Therefor, if
-- a `screen` is already specified, then it will be ignored when the rule is
-- executed. Properties also have their own priorities. For example, the
-- `titlebar` and `border_width` need to be applied before the `x` and `y`
-- positions are set. Otherwise, it will be off or the client will shift
-- upward everytime Awesome is restarted. A rule source *cannot* change this.
-- It is up to the callback to be aware of the dependencies and avoid to
-- introduce issues. For example, if the source wants to set a `screen`, it has
-- to check if the `tag`, `tags` or `new_tag` are on that `screen` or remove
-- those properties. Otherwise, they will be ignored once the rule is applied.
--
-- @tparam string name The provider name. It must be unique.
-- @tparam function callback The callback that is called to produce properties.
-- @tparam client callback.c The client
-- @tparam table callback.properties The current properties. The callback should
--  add to and overwrite properties in this table
-- @tparam table callback.callbacks A table of all callbacks scheduled to be
--  executed after the main properties are applied.
-- @tparam[opt={}] table depends_on A list of names of sources this source depends on
--  (sources that must be executed *before* `name`.
-- @tparam[opt={}] table precede A list of names of sources this source have a
--  priority over.
-- @treturn boolean Returns false if a dependency conflict was found.
-- @staticfct ruled.client.add_rule_source

function module.add_rule_source(name, cb, ...)
    local function callback(_, ...)
        cb(...)
    end
    return crules:add_matching_function(name, callback, ...)
end

--- The default `ruled.client` source.
--
-- It is called `awful.rules` for historical reasons.
--
-- **Has priority over:**
--
-- *nothing*
--
-- @rulesources awful.rules

crules:add_matching_rules("awful.rules", {}, {"awful.spawn"}, {})

-- Add startup_id overridden properties
local function apply_spawn_rules(c, props, callbacks)
    if c.startup_id and aspawn.snid_buffer[c.startup_id] then
        local snprops, sncb = unpack(aspawn.snid_buffer[c.startup_id])

        -- The SNID tag(s) always have precedence over the rules one(s)
        if snprops.tag or snprops.tags or snprops.new_tag then
            props.tag, props.tags, props.new_tag = nil, nil, nil
        end

        gtable.crush(props, snprops)
        gtable.merge(callbacks, sncb)
    end
end

--- The rule source for clients spawned by `awful.spawn`.
--
-- **Has priority over:**
--
-- * `awful.rules`
--
-- @rulesources awful.spawn

module.add_rule_source("awful.spawn", apply_spawn_rules, {}, {"awful.rules"})

local function apply_singleton_rules(c, props, callbacks)
    local persis_id, info = c.single_instance_id, nil

    -- This is a persistent property set by `awful.spawn`
    if capi.awesome.startup and persis_id then
        info = aspawn.single_instance_manager.by_uid[persis_id]
    elseif c.startup_id then
        info = aspawn.single_instance_manager.by_snid[c.startup_id]
        aspawn.single_instance_manager.by_snid[c.startup_id] = nil
    elseif aspawn.single_instance_manager.by_pid[c.pid] then
        info = aspawn.single_instance_manager.by_pid[c.pid].matcher(c) and
            aspawn.single_instance_manager.by_pid[c.pid] or nil
    end

    if info then
        c.single_instance_id = info.hash
        if info.rules then
            gtable.crush(props, info.rules)
        end
        table.insert(callbacks, info.callback)
        table.insert(info.instances, c)

        -- Prevent apps with multiple clients from re-using this too often in
        -- the first 30 seconds before the PID is cleared.
        aspawn.single_instance_manager.by_pid[c.pid] = nil
    end
end

--- The rule source for clients spawned by `awful.spawn.once` and `single_instance`.
--
-- **Has priority over:**
--
-- * `awful.rules`
--
-- **Depends on:**
--
-- * `awful.spawn`
--
-- @rulesources awful.spawn_once

module.add_rule_source("awful.spawn_once", apply_singleton_rules, {"awful.spawn"}, {"awful.rules"})

local function add_to_tag(c, t)
    if not t then return end

    local tags = c:tags()
    table.insert(tags, t)
    c:tags(tags)
end

--- Extra rules properties.
--
-- These properties are used in the rules only and are not sent to the client
-- afterward.
--
-- To add a new properties, just do:
--
--    function ruled.client.extra_properties.my_new_property(c, value, props)
--        -- do something
--    end
--
-- By default, the table has the following functions:
--
-- * geometry
-- * placement
--
-- @tfield table ruled.client.extra_properties
module.extra_properties = {}

--- Extra high priority properties.
--
-- Some properties, such as anything related to tags, geometry or focus, will
-- cause a race condition if set in the main property section. This is why
-- they have a section for them.
--
-- To add a new properties, just do:
--
--    function ruled.client.high_priority_properties.my_new_property(c, value, props)
--        -- do something
--    end
--
-- By default, the table has the following functions:
--
-- * tag
-- * new_tag
--
-- @tfield table ruled.client.high_priority_properties
module.high_priority_properties = {}

--- Delayed properties.
-- Properties applied after all other categories.
-- @tfield table ruled.client.delayed_properties
-- By default, the table has the following functions:
--
-- * switch_to_tags
module.delayed_properties = {}

local force_ignore = {
    titlebars_enabled=true, focus=true, screen=true, x=true,
    y=true, width=true, height=true, geometry=true,placement=true,
    border_width=true,floating=true,size_hints_honor=true
}

function module.high_priority_properties.tag(c, value, props)
    if value then
        if type(value) == "string" then
            local name = value
            value = atag.find_by_name(c.screen, value)
            if not value and not props.screen then
                value = atag.find_by_name(nil, name)
            end
            if not value then
                gdebug.print_error("ruled.client-rule specified "
                    .. "tag = '" .. name .. "', but no such tag exists")
                return
            end
        end

        -- In case the tag has been forced to another screen, move the client
        if c.screen ~= value.screen then
            c.screen = value.screen
            props.screen = value.screen -- In case another rule query it
        end

        c:tags{ value }
    end
end

function module.delayed_properties.switch_to_tags(c, value)
    if not value then return end
    atag.viewmore(c:tags(), c.screen)
end

function module.delayed_properties.switchtotag(c, value)
    gdebug.deprecate("Use switch_to_tags instead of switchtotag", {deprecated_in=5})

    module.delayed_properties.switch_to_tags(c, value)
end

function module.extra_properties.geometry(c, _, props)
    local cur_geo = c:geometry()

    local new_geo = type(props.geometry) == "function"
        and props.geometry(c, props) or props.geometry or {}

    for _, v in ipairs {"x", "y", "width", "height"} do
        new_geo[v] = type(props[v]) == "function" and props[v](c, props)
            or props[v] or new_geo[v] or cur_geo[v]
    end

    c:geometry(new_geo) --TODO use request::geometry
end

function module.high_priority_properties.new_tag(c, value, props)
    local ty = type(value)
    local t = nil

    if ty == "boolean" then
        -- Create a new tag named after the client class
        t = atag.add(c.class or "N/A", {screen=c.screen, volatile=true})
    elseif ty == "string" then
        -- Create a tag named after "value"
        t = atag.add(value, {screen=c.screen, volatile=true})
    elseif ty == "table" then
        -- Assume a table of tags properties. Set the right screen, but
        -- avoid editing the original table
        local values = value.screen and value or gtable.clone(value)
        values.screen = values.screen or c.screen

        t = atag.add(value.name or c.class or "N/A", values)

        -- In case the tag has been forced to another screen, move the client
        c.screen = t.screen
        props.screen = t.screen -- In case another rule query it
    else
        assert(false)
    end

    add_to_tag(c, t)

    return t
end

function module.extra_properties.placement(c, value, props)
    -- Avoid problems
    if capi.awesome.startup and
      (c.size_hints.user_position or c.size_hints.program_position) then
        return
    end

    local ty = type(value)

    local args = {
        honor_workarea = props.honor_workarea ~= false,
        honor_padding  = props.honor_padding ~= false
    }

    if ty == "function" or (ty == "table" and
        getmetatable(value) and getmetatable(value).__call
    ) then
        value(c, args)
    elseif ty == "string" and a_place[value] then
        a_place[value](c, args)
    end
end

function module.high_priority_properties.tags(c, value, props)
    local current = c:tags()

    local tags, s = {}, nil

    for _, t in ipairs(value) do
        if type(t) == "string" then
            t = atag.find_by_name(c.screen, t)
        end

        if t and ((not s) or t.screen == s) then
            table.insert(tags, t)
            s = s or t.screen
        end
    end

    if s and s ~= c.screen then
        c.screen = s
        props.screen = s -- In case another rule query it
    end

    if #current == 0 or (value[1] and value[1].screen ~= current[1].screen) then
        c:tags(tags)
    else
        c:tags(gtable.merge(current, tags))
    end
end

--- Apply properties and callbacks to a client.
-- @client c The client.
-- @tab props Properties to apply.
-- @tab[opt] callbacks Callbacks to apply.
-- @staticfct ruled.client.execute
-- @request client titlebars rules granted The `titlebars_enabled` is set in the
--  rules.

crules._execute = function(_, c, props, callbacks)

    -- Set the default buttons and keys
    local btns = amouse._get_client_mousebindings()
    local keys = akeyboard._get_client_keybindings()
    props.keys    = props.keys or keys
    props.buttons = props.buttons or btns

    -- Border width will also cause geometry related properties to fail
    if props.border_width then
        c.border_width = type(props.border_width) == "function" and
            props.border_width(c, props) or props.border_width
    end

    -- This has to be done first, as it will impact geometry related props.
    if props.titlebars_enabled and (type(props.titlebars_enabled) ~= "function"
            or props.titlebars_enabled(c,props)) then
        c:emit_signal("request::titlebars", "rules", {properties=props})
        c._request_titlebars_called = true
    end

    -- Size hints will be re-applied when setting width/height unless it is
    -- disabled first
    if props.size_hints_honor ~= nil then
        c.size_hints_honor = type(props.size_hints_honor) == "function" and props.size_hints_honor(c,props)
            or props.size_hints_honor
    end

    -- Geometry will only work if floating is true, otherwise the "saved"
    -- geometry will be restored.
    if props.floating ~= nil then
        c.floating = type(props.floating) == "function" and props.floating(c,props)
            or props.floating
    end

    -- Before requesting a tag, make sure the screen is right
    if props.screen then
        c.screen = type(props.screen) == "function" and capi.screen[props.screen(c,props)]
            or capi.screen[props.screen]
    end

    -- Some properties need to be handled first. For example, many properties
    -- require that the client is tagged, this isn't yet the case.
    for prop, handler in pairs(module.high_priority_properties) do
        local value = props[prop]
        if value ~= nil then
            if type(value) == "function" then
                value = value(c, props)
            end
            handler(c, value, props)
        end
    end

    -- Make sure the tag is selected before the main rules are called.
    -- Otherwise properties like "urgent" or "focus" may fail (if they were
    -- overridden by other callbacks).
    -- Previously this was done in a second client.manage callback, but caused
    -- a race condition where the order of modules being loaded would change
    -- the outcome.
    c:emit_signal("request::tag", nil, {reason="rules", screen = c.screen})

    -- By default, rc.lua uses no_overlap+no_offscreen placement. This has to
    -- be executed before x/y/width/height/geometry as it would otherwise
    -- always override the user specified position with the default rule.
    if props.placement then
        -- It may be a function, so this one doesn't execute it like others
        module.extra_properties.placement(c, props.placement, props)
    end

    -- Handle the geometry (since tags and screen are set).
    if props.height or props.width or props.x or props.y or props.geometry then
        module.extra_properties.geometry(c, nil, props)
    end

    -- Apply the remaining properties (after known race conditions are handled).
    for property, value in pairs(props) do
        if property ~= "focus" and property ~= "shape" and type(value) == "function" then
            value = value(c, props)
        end

        local ignore = module.high_priority_properties[property] or
            module.delayed_properties[property] or force_ignore[property]

        if not ignore then
            if module.extra_properties[property] then
                module.extra_properties[property](c, value, props)
            elseif type(c[property]) == "function" then
                c[property](c, value)
            else
                c[property] = value
            end
        end
    end

    -- Apply all callbacks.
    if callbacks then
        for _, callback in pairs(callbacks) do
            protected_call(callback, c)
        end
    end

    -- Apply the delayed properties
    for prop, handler in pairs(module.delayed_properties) do
        if not force_ignore[prop] then
            local value = props[prop]
            if value ~= nil then
                if type(value) == "function" then
                    value = value(c, props)
                end
                handler(c, value, props)
            end
        end
    end

    -- Do this at last so we do not erase things done by the focus signal.
    if props.focus and (type(props.focus) ~= "function" or props.focus(c)) then
        c:emit_signal('request::activate', "rules", {raise=not capi.awesome.startup})
    end
end

function module.execute(...) crules:_execute(...) end

-- TODO v5 deprecate this
function module.completed_with_payload_callback(c, props, callbacks)
    module.execute(c, props, callbacks)
end

gobject._setup_class_signals(module)

capi.client.connect_signal("request::manage", module.apply)

-- Request rules to be added **after** all modules are loaded, but before the
-- clients are managed. This allows module to listen to rules being added and
-- either modify them or add their own in the right order.
local function request_rules()
    module.emit_signal("request::rules")
end

capi.client.connect_signal("scanning", request_rules)

--@DOC_rule_COMMON@

return setmetatable(module, {
    __newindex = function(_, k, v)
        if k == "rules" then
            gdebug.deprecate(
                "Use ruled.client.append_rules instead awful.rules.rules",
                {deprecated_in=5}
            )

            -- Clearing the rule was supported, so it still has to be. This is
            -- a bad idea. There is no plan to make this API public.
            if not next(v) then
                -- It isn't possible to just set it to {}, there is other
                -- references to the table.
                for k2 in pairs(crules._matching_rules["awful.rules"]) do
                    crules._matching_rules["awful.rules"][k2] = nil
                end
            else
                crules:append_rules("awful.rules", v)
            end
        else
            rawset(k, v)
        end
    end,
    __index = function(_, k)
        if k == "rules" then
            gdebug.deprecate(
                "Accessing `ruled.rules` isn't recommended, to modify rules, "..
                "use `ruled.client.remove_rule()` and add a new one.",
                {deprecated_in=5}
            )

            if not crules._matching_rules["awful.rules"] then
                crules:add_matching_rules("awful.rules", {}, {}, {})
            end

            return crules._matching_rules["awful.rules"]
        end
    end
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
