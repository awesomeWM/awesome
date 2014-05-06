---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local client = client
local table = table
local type = type
local ipairs = ipairs
local pairs = pairs
local aclient = require("awful.client")
local atag = require("awful.tag")

--- Apply rules to clients at startup.
-- awful.rules
local rules = {}

--- This is the global rules table.
-- <p>You should fill this table with your rule and properties to apply.
-- For example, if you want to set xterm maximized at startup, you can add:
-- <br/>
-- <code>
-- { rule = { class = "xterm" },
--   properties = { maximized_vertical = true, maximized_horizontal = true } }
-- </code>
-- </p>
-- <p>If you want to set mplayer floating at startup, you can add:
-- <br/>
-- <code>
-- { rule = { name = "MPlayer" },
--   properties = { floating = true } }
-- </code>
-- </p>
-- <p>If you want to put Firefox on a specific tag at startup, you
-- can add:
-- <br/>
-- <code>
-- { rule = { instance = "firefox" },
--   properties = { tag = mytagobject } }
-- </code>
-- </p>
-- <p>If you want to put Emacs on a specific tag at startup, and
-- immediately switch to that tag you can add:
-- <br/>
-- <code>
-- { rule = { class = "Emacs" },
--   properties = { tag = mytagobject, switchtotag = true } }
-- </code>
-- </p>
-- <p>If you want to apply a custom callback to execute when a rule matched,
-- for example to pause playing music from mpd when you start dosbox, you
-- can add:
-- <br/>
-- <code>
-- { rule = { class = "dosbox" },
--   callback = function(c)
--      awful.util.spawn('mpc pause')
--   end }
-- </code>
-- </p>
-- <p>Note that all "rule" entries need to match. If any of the entry does not
-- match, the rule won't be applied.</p>
-- <p>If a client matches multiple rules, their applied in the order they are
-- put in this global rules table. If the value of a rule is a string, then the
-- match function is used to determine if the client matches the rule.</p>
-- <p>If the value of a property is a function, that function gets called and
-- function's return value is used for the property.</p>
--
-- <p> To match multiple clients to a rule one need to use slightly different 
-- syntax:
-- <br/>
-- <code>
-- { rule_any = { class = { "MPlayer", "Nitrogen" }, instance = { "xterm" } },
--   properties = { floating = true } }
-- </code>
-- </p>
--
-- <p> To match multiple clients with an exception one can couple 'except' or
-- 'except_any' with the rules:
-- <br/>
-- <code>
-- { rule = { class = "Firefox" },
--   except = { instance = "Navigator" },
--   properties = {floating = true},
-- },
-- </code>
-- <br/>
-- <code>
-- { rule_any = { class = { "Pidgin", "Xchat" } },
--   except_any = { role = { "conversation" } },
--   properties = { tag = tags[1][1] }
-- }
-- <br/>
-- <code>
-- { rule = {},
--   except_any = { class = { "Firefox", "Vim" } },
--   properties = { floating = true }
-- }
-- </code>
-- </p>
--
-- @class table
-- @name rules
rules.rules = {}

--- Check if a client matches a rule.
-- @param c The client.
-- @param rule The rule to check.
-- @return True if it matches, false otherwise.
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
-- @param c The client.
-- @param rule The rule to check.
-- @return True if at least one rule is matched, false otherwise.
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

--- Get list of matching rules for a client.
-- @param c The client.
-- @param rule The rules to check. List with "rule", "rule_any", "except" and
-- "except_any" keys.
-- @return The list of matched rules.
function rules.matching_rules(c, _rules)
    local result = {}
    for _, entry in ipairs(_rules) do
        if (rules.match(c, entry.rule) or rules.match_any(c, entry.rule_any)) and
            (not rules.match(c, entry.except) and not rules.match_any(c, entry.except_any)) then
            table.insert(result, entry)
        end
    end
    return result
end

--- Check if a client matches a given set of rules.
-- @param c The client.
-- @param rule The rules to check. List with "rule", "rule_any", "except" and
-- "except_any" keys.
-- @return True if at least one rule is matched, false otherwise.
function rules.does_match(c, rules)
    local result = rules.matching_rules(c, rules)
    return #result == 0 and false or result
end

--- Apply awful.rules.rules to a client.
-- @param c The client.
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


--- Apply properties and callbacks to a client.
-- @param c The client.
-- @param props Properties to apply.
-- @param callbacks Callbacks to apply.
function rules.execute(c, props, callbacks)
    for property, value in pairs(props) do
        if property ~= "focus" and type(value) == "function" then
            value = value(c)
        end
        if property == "floating" then
            aclient.floating.set(c, value)
        elseif property == "tag" then
            c.screen = atag.getscreen(value)
            c:tags({ value })
        elseif property == "switchtotag" and value and props.tag then
            atag.viewonly(props.tag)
        elseif property == "height" or property == "width" or
                property == "x" or property == "y" then
            local geo = c:geometry();
            geo[property] = value
            c:geometry(geo);
        elseif property == "focus" then
            -- This will be handled below
        elseif type(c[property]) == "function" then
            c[property](c, value)
        else
            c[property] = value
        end
    end

    -- If untagged, stick the client on the current one.
    if #c:tags() == 0 then
        atag.withcurrent(c)
    end

    -- Apply all callbacks.
    for i, callback in pairs(callbacks) do
        callback(c)
    end

    -- Do this at last so we do not erase things done by the focus
    -- signal.
    if props.focus and (type(props.focus) ~= "function" or props.focus(c)) then
        c:emit_signal('request::activate',"rules")
    end
end

client.connect_signal("manage", rules.apply)
client.disconnect_signal("manage", atag.withcurrent)

return rules

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
