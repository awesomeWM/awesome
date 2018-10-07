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
--     { rule = { class = "xterm" },
--       properties = { maximized_vertical = true, maximized_horizontal = true } }
--
-- If you want to set mplayer floating at startup, you can add:
--
--     { rule = { name = "MPlayer" },
--       properties = { floating = true } }
--
-- If you want to put Firefox on a specific tag at startup, you can add:
--
--     { rule = { instance = "firefox" },
--       properties = { tag = mytagobject } }
--
-- Alternatively, you can specify the tag by name:
--
--     { rule = { instance = "firefox" },
--       properties = { tag = "3" } }
--
-- If you want to put Thunderbird on a specific screen at startup, use:
--
--     { rule = { instance = "Thunderbird" },
--       properties = { screen = 1 } }
--
-- Assuming that your X11 server supports the RandR extension, you can also specify
-- the screen by name:
--
--     { rule = { instance = "Thunderbird" },
--       properties = { screen = "VGA1" } }
--
-- If you want to put Emacs on a specific tag at startup, and immediately switch
-- to that tag you can add:
--
--     { rule = { class = "Emacs" },
--       properties = { tag = mytagobject, switchtotag = true } }
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
-- Applicable client properties
-- ===
--
-- The table below holds the list of default client properties along with
-- some extra properties that are specific to the rules. Note that any property
-- can be set in the rules and interpreted by user provided code. This table
-- only represent those offered by default.
--
--@DOC_rules_index_COMMON@
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @module awful.rules
---------------------------------------------------------------------------

-- Grab environment we need
local client = client
local awesome = awesome
local screen = screen
local table = table
local type = type
local ipairs = ipairs
local pairs = pairs
local atag = require("awful.tag")
local gtable = require("gears.table")
local a_place = require("awful.placement")
local protected_call = require("gears.protected_call")
local aspawn = require("awful.spawn")
local gsort = require("gears.sort")
local gdebug = require("gears.debug")
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local rules = {}

--- This is the global rules table.
rules.rules = {}

--- Check if a client matches a rule.
-- @client c The client.
-- @tab rule The rule to check.
-- @treturn bool True if it matches, false otherwise.
function rules.match(c, rule)
    if not rule then return false end
    for field, value in pairs(rule) do
        if c[field] then
            if type(c[field]) == "string" then
                if not c[field]:match(value) and c[field] ~= value then
                    return false
                end
            elseif c[field] ~= value then
                return false
            end
        else
            return false
        end
    end
    return true
end

--- Check if a client matches any part of a rule.
-- @client c The client.
-- @tab rule The rule to check.
-- @treturn bool True if at least one rule is matched, false otherwise.
function rules.match_any(c, rule)
    if not rule then return false end
    for field, values in pairs(rule) do
        if c[field] then
            for _, value in ipairs(values) do
                if c[field] == value then
                    return true
                elseif type(c[field]) == "string" and c[field]:match(value) then
                    return true
                end
            end
        end
    end
    return false
end

--- Does a given rule entry match a client?
-- @client c The client.
-- @tab entry Rule entry (with keys `rule`, `rule_any`, `except` and/or
--   `except_any`).
-- @treturn bool
function rules.matches(c, entry)
    return (rules.match(c, entry.rule) or rules.match_any(c, entry.rule_any)) and
        (not rules.match(c, entry.except) and not rules.match_any(c, entry.except_any))
end

--- Get list of matching rules for a client.
-- @client c The client.
-- @tab _rules The rules to check. List with "rule", "rule_any", "except" and
--   "except_any" keys.
-- @treturn table The list of matched rules.
function rules.matching_rules(c, _rules)
    local result = {}
    for _, entry in ipairs(_rules) do
        if (rules.matches(c, entry)) then
            table.insert(result, entry)
        end
    end
    return result
end

--- Check if a client matches a given set of rules.
-- @client c The client.
-- @tab _rules The rules to check. List of tables with `rule`, `rule_any`,
--   `except` and `except_any` keys.
-- @treturn bool True if at least one rule is matched, false otherwise.
function rules.matches_list(c, _rules)
    for _, entry in ipairs(_rules) do
        if (rules.matches(c, entry)) then
            return true
        end
    end
    return false
end


-- Contains the sources.
-- The elements are ordered "first in, first executed". Thus, the higher the
-- index, the higher the priority. Each entry is a table with a `name` and a
-- `callback` field. This table is exposed for debugging purpose. The API
-- is private and should be modified using the public accessors.
local rule_sources = {}
local rule_source_sort = gsort.topological()

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
function rules.add_rule_source(name, callback, depends_on, precede)
    depends_on = depends_on  or {}
    precede    = precede or {}
    assert(type( depends_on ) == "table")
    assert(type( precede    ) == "table")

    for _, v in ipairs(rule_sources) do
        -- Names must be unique
        assert(
            v.name ~= name,
            "Name must be unique, but '" .. name .. "' was already registered."
        )
    end

    local new_sources = rule_source_sort:clone()

    new_sources:prepend(name, precede    )
    new_sources:append (name, depends_on )

    local res, err = new_sources:sort()

    if err then
        gdebug.print_warning("Failed to add the rule source: "..err)
        return false
    end

    -- Only replace the source once the additions have been proven safe
    rule_source_sort = new_sources

    local callbacks = {}

    -- Get all callbacks for *existing* sources.
    -- It is important to remember that names can be used in the sorting even
    -- if the source itself doesn't (yet) exists.
    for _, v in ipairs(rule_sources) do
        callbacks[v.name] = v.callback
    end

    rule_sources = {}
    callbacks[name]    = callback

    for _, v in ipairs(res) do
        if callbacks[v] then
            table.insert(rule_sources, 1, {
                callback = callbacks[v],
                name     = v
            })
        end
    end

    return true
end

--- Remove a source.
-- @tparam string name The source name.
-- @treturn boolean If the source was removed
function rules.remove_rule_source(name)
    rule_source_sort:remove(name)

    for k, v in ipairs(rule_sources) do
        if v.name == name then
            table.remove(rule_sources, k)
            return true
        end
    end

    return false
end

-- Add the rules properties
local function apply_awful_rules(c, props, callbacks)
    for _, entry in ipairs(rules.matching_rules(c, rules.rules)) do
        gtable.crush(props,entry.properties or {})

        if entry.callback then
            table.insert(callbacks, entry.callback)
        end
    end
end

--- The default `awful.rules` source.
--
-- **Has priority over:**
--
-- *nothing*
--
-- @rulesources awful.rules

rules.add_rule_source("awful.rules", apply_awful_rules, {"awful.spawn"}, {})

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

rules.add_rule_source("awful.spawn", apply_spawn_rules, {}, {"awful.rules"})

local function apply_singleton_rules(c, props, callbacks)
    local persis_id, info = c.single_instance_id, nil

    -- This is a persistent property set by `awful.spawn`
    if awesome.startup and persis_id then
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
        gtable.crush(props, info.rules)
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

rules.add_rule_source("awful.spawn_once", apply_singleton_rules, {"awful.spawn"}, {"awful.rules"})

--- Apply awful.rules.rules to a client.
-- @client c The client.
function rules.apply(c)
    local callbacks, props = {}, {}
    for _, v in ipairs(rule_sources) do
        v.callback(c, props, callbacks)
    end

    rules.execute(c, props, callbacks)
end

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
--    function awful.rules.extra_properties.my_new_property(c, value, props)
--        -- do something
--    end
--
-- By default, the table has the following functions:
--
-- * geometry
-- * placement
--
-- @tfield table awful.rules.extra_properties
rules.extra_properties = {}

--- Extra high priority properties.
--
-- Some properties, such as anything related to tags, geometry or focus, will
-- cause a race condition if set in the main property section. This is why
-- they have a section for them.
--
-- To add a new properties, just do:
--
--    function awful.rules.high_priority_properties.my_new_property(c, value, props)
--        -- do something
--    end
--
-- By default, the table has the following functions:
--
-- * tag
-- * new_tag
--
-- @tfield table awful.rules.high_priority_properties
rules.high_priority_properties = {}

--- Delayed properties.
-- Properties applied after all other categories.
-- @tfield table awful.rules.delayed_properties
-- By default, the table has the following functions:
--
-- * switch_to_tags
rules.delayed_properties = {}

local force_ignore = {
    titlebars_enabled=true, focus=true, screen=true, x=true,
    y=true, width=true, height=true, geometry=true,placement=true,
    border_width=true,floating=true,size_hints_honor=true
}

function rules.high_priority_properties.tag(c, value, props)
    if value then
        if type(value) == "string" then
            local name = value
            value = atag.find_by_name(c.screen, value)
            if not value and not props.screen then
                value = atag.find_by_name(nil, value)
            end
            if not value then
                require("gears.debug").print_error("awful.rules-rule specified "
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

function rules.delayed_properties.switch_to_tags(c, value)
    if not value then return end
    atag.viewmore(c:tags(), c.screen)
end

function rules.delayed_properties.switchtotag(c, value)
    gdebug.deprecate("Use switch_to_tags instead of switchtotag", {deprecated_in=5})

    rules.delayed_properties.switch_to_tags(c, value)
end

function rules.extra_properties.geometry(c, _, props)
    local cur_geo = c:geometry()

    local new_geo = type(props.geometry) == "function"
        and props.geometry(c, props) or props.geometry or {}

    for _, v in ipairs {"x", "y", "width", "height"} do
        new_geo[v] = type(props[v]) == "function" and props[v](c, props)
            or props[v] or new_geo[v] or cur_geo[v]
    end

    c:geometry(new_geo) --TODO use request::geometry
end

function rules.high_priority_properties.new_tag(c, value, props)
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

function rules.extra_properties.placement(c, value, props)
    -- Avoid problems
    if awesome.startup and
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

function rules.high_priority_properties.tags(c, value, props)
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
function rules.execute(c, props, callbacks)
    -- This has to be done first, as it will impact geometry related props.
    if props.titlebars_enabled and (type(props.titlebars_enabled) ~= "function"
            or props.titlebars_enabled(c,props)) then
        c:emit_signal("request::titlebars", "rules", {properties=props})
        c._request_titlebars_called = true
    end

    -- Border width will also cause geometry related properties to fail
    if props.border_width then
        c.border_width = type(props.border_width) == "function" and
            props.border_width(c, props) or props.border_width
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
        c.screen = type(props.screen) == "function" and screen[props.screen(c,props)]
            or screen[props.screen]
    end

    -- Some properties need to be handled first. For example, many properties
    -- require that the client is tagged, this isn't yet the case.
    for prop, handler in pairs(rules.high_priority_properties) do
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
    c:emit_signal("request::tag", nil, {reason="rules"})

    -- By default, rc.lua uses no_overlap+no_offscreen placement. This has to
    -- be executed before x/y/width/height/geometry as it would otherwise
    -- always override the user specified position with the default rule.
    if props.placement then
        -- It may be a function, so this one doesn't execute it like others
        rules.extra_properties.placement(c, props.placement, props)
    end

    -- Handle the geometry (since tags and screen are set).
    if props.height or props.width or props.x or props.y or props.geometry then
        rules.extra_properties.geometry(c, nil, props)
    end

    -- Apply the remaining properties (after known race conditions are handled).
    for property, value in pairs(props) do
        if property ~= "focus" and type(value) == "function" then
            value = value(c, props)
        end

        local ignore = rules.high_priority_properties[property] or
            rules.delayed_properties[property] or force_ignore[property]

        if not ignore then
            if rules.extra_properties[property] then
                rules.extra_properties[property](c, value, props)
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
    for prop, handler in pairs(rules.delayed_properties) do
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
        c:emit_signal('request::activate', "rules", {raise=true})
    end
end

-- TODO v5 deprecate this
function rules.completed_with_payload_callback(c, props, callbacks)
    rules.execute(c, props, callbacks)
end

client.connect_signal("manage", rules.apply)

return rules

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
