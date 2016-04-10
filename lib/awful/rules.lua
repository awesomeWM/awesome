---------------------------------------------------------------------------
--- Apply rules to clients at startup.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.rules
---------------------------------------------------------------------------

-- Grab environment we need
local client = client
local screen = screen
local table = table
local type = type
local ipairs = ipairs
local pairs = pairs

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
      properties = { tag = tags[1][1] }
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


--- Apply properties and callbacks to a client.
-- @client c The client.
-- @tab props Properties to apply.
-- @tab[opt] callbacks Callbacks to apply.
function rules.execute(c, props, callbacks)
    for property, value in pairs(props) do
        if property ~= "focus" and type(value) == "function" then
            value = value(c)
        end
        if property == "screen" then
            -- Support specifying screens by name ("VGA1")
            c.screen = screen[value]
        elseif property == "tag" then
            c.screen = value.screen
            c:tags({ value })
        elseif property == "switchtotag" and value and props.tag then
            props.tag:view_only()
        elseif property == "height" or property == "width" or
                property == "x" or property == "y" then
            local geo = c:geometry();
            geo[property] = value
            c:geometry(geo);
        elseif property == "focus" then
            -- This will be handled below
            (function() end)() -- I haven't found a nice way to silence luacheck here
        elseif type(c[property]) == "function" then
            c[property](c, value)
        else
            c[property] = value
        end
    end

    -- Apply all callbacks.
    if callbacks then
        for _, callback in pairs(callbacks) do
            callback(c)
        end
    end

    -- Do this at last so we do not erase things done by the focus signal.
    if props.focus and (type(props.focus) ~= "function" or props.focus(c)) then
        c:emit_signal('request::activate', "rules", {raise=true})
    end
end

function rules.completed_with_payload_callback(c, props)
    rules.execute(c, props, type(props.callback) == "function" and
        {props.callback} or props.callback )
end

client.connect_signal("spawn::completed_with_payload", rules.completed_with_payload_callback)

client.connect_signal("manage", rules.apply)

return rules

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
