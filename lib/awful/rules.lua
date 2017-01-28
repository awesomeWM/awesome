---------------------------------------------------------------------------
--- Apply rules to clients at startup.
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
local util = require("awful.util")
local a_place = require("awful.placement")
local protected_call = require("gears.protected_call")

local rules = {}

--[[--
This is the global rules table.

You should fill this table with your rule and properties to apply.
For example, if you want to set xterm maximized at startup, you can add:

    { rule = { class = "xterm" },
      properties = { maximized_vertical = true, maximized_horizontal = true } }

If you want to set mplayer floating at startup, you can add:

    { rule = { name = "MPlayer" },
      properties = { floating = true } }

If you want to put Firefox on a specific tag at startup, you can add:

    { rule = { instance = "firefox" },
      properties = { tag = mytagobject } }

Alternatively, you can specify the tag by name:

    { rule = { instance = "firefox" },
      properties = { tag = "3" } }

If you want to put Thunderbird on a specific screen at startup, use:

    { rule = { instance = "Thunderbird" },
      properties = { screen = 1 } }

Assuming that your X11 server supports the RandR extension, you can also specify
the screen by name:

    { rule = { instance = "Thunderbird" },
      properties = { screen = "VGA1" } }

If you want to put Emacs on a specific tag at startup, and immediately switch
to that tag you can add:

    { rule = { class = "Emacs" },
      properties = { tag = mytagobject, switchtotag = true } }

If you want to apply a custom callback to execute when a rule matched,
for example to pause playing music from mpd when you start dosbox, you
can add:

    { rule = { class = "dosbox" },
      callback = function(c)
         awful.spawn('mpc pause')
      end }

Note that all "rule" entries need to match. If any of the entry does not
match, the rule won't be applied.

If a client matches multiple rules, they are applied in the order they are
put in this global rules table. If the value of a rule is a string, then the
match function is used to determine if the client matches the rule.

If the value of a property is a function, that function gets called and
function's return value is used for the property.

To match multiple clients to a rule one need to use slightly different
syntax:

    { rule_any = { class = { "MPlayer", "Nitrogen" }, instance = { "xterm" } },
      properties = { floating = true } }

To match multiple clients with an exception one can couple `rules.except` or
`rules.except_any` with the rules:

    { rule = { class = "Firefox" },
      except = { instance = "Navigator" },
      properties = {floating = true},
    },

    { rule_any = { class = { "Pidgin", "Xchat" } },
      except_any = { role = { "conversation" } },
      properties = { tag = "1" }
    }

    { rule = {},
      except_any = { class = { "Firefox", "Vim" } },
      properties = { floating = true }
    }
]]--
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

--- Apply awful.rules.rules to a client.
-- @client c The client.
function rules.apply(c)

    local props = {}
    local callbacks = {}

    for _, entry in ipairs(rules.matching_rules(c, rules.rules)) do
        if entry.properties then
            for property, value in pairs(entry.properties) do
                props[property] = value
            end
        end
        if entry.callback then
            table.insert(callbacks, entry.callback)
        end
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
-- * switchtotag
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
rules.delayed_properties = {}

local force_ignore = {
    titlebars_enabled=true, focus=true, screen=true, x=true,
    y=true, width=true, height=true, geometry=true,placement=true,
    border_width=true,floating=true,size_hints_honor=true
}

function rules.high_priority_properties.tag(c, value, props)
    if value then
        if type(value) == "string" then
            value = atag.find_by_name(c.screen, value)
        end

        -- In case the tag has been forced to another screen, move the client
        if c.screen ~= value.screen then
            c.screen = value.screen
            props.screen = value.screen -- In case another rule query it
        end

        c:tags{ value }
    end
end

function rules.delayed_properties.switchtotag(c, value)
    if not value then return end

    local selected_tags = {}

    for _,v in ipairs(c.screen.selected_tags) do
        selected_tags[v] = true
    end

    local tags = c:tags()

    for _, t in ipairs(tags) do
        t.selected = true
        selected_tags[t] = nil
    end

    for t in pairs(selected_tags) do
        t.selected = false
    end
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

--- Create a new tag based on a rule.
-- @tparam client c The client
-- @tparam boolean|function|string value The value.
-- @tparam table props The properties.
-- @treturn tag The new tag
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
        local values = value.screen and value or util.table.clone(value)
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
        c:tags(util.table.merge(current, tags))
    end
end

--- Apply properties and callbacks to a client.
-- @client c The client.
-- @tab props Properties to apply.
-- @tab[opt] callbacks Callbacks to apply.
function rules.execute(c, props, callbacks)
    -- This has to be done first, as it will impact geometry related props.
    if props.titlebars_enabled then
        c:emit_signal("request::titlebars", "rules", {properties=props})
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
    -- depend that the client is tagged, this isn't yet the case.
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
    -- Otherwise properties like "urgent" or "focus" may fail because they
    -- will be overiden by various callbacks.
    -- Previously, this was done in a second client.manage callback, but caused
    -- a race condition where the order the require() would change the output.
    c:emit_signal("request::tag", nil, {reason="rules"})

    -- By default, rc.lua use no_overlap+no_offscreen placement. This has to
    -- be executed before x/y/width/height/geometry as it would otherwise
    -- always override the user specified position with the default rule.
    if props.placement then
        -- It may be a function, so this one doesn't execute it like others
        rules.extra_properties.placement(c, props.placement, props)
    end

    -- Now that the tags and screen are set, handle the geometry
    if props.height or props.width or props.x or props.y or props.geometry then
        rules.extra_properties.geometry(c, nil, props)
    end

    -- As most race conditions should now have been avoided, apply the remaining
    -- properties.
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

function rules.completed_with_payload_callback(c, props, callbacks)
    rules.execute(c, props, callbacks)
end

client.connect_signal("spawn::completed_with_payload", rules.completed_with_payload_callback)

client.connect_signal("manage", rules.apply)

return rules

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
