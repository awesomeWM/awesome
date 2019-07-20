--- A module to build a set of properties based on a graph of rules.
--
-- Sources
-- =======
--
-- This module holds the business logic used by `awful.rules`. It provides an
-- object on which one can add sets of rules or, alternatively, functions.
-- In this module, the sets of rules or custom functions are called sources.
--
-- The sources are used to build a property table. Once all sources are
-- evaluated, the `:apply()` method will set the properties on the target
-- object.
--
-- Sources can have dependencies between them and the property table can only
-- be built if the sources graph can be resolved.
--
-- Rules
-- =====
--
-- The `rules` sources themselves are composed, as the name imply, of a set of
-- rule. A rule is a table with a `properties` *or* `callbacks` attribute along
-- with either `rule` or `rule_any`. It is also possible to add an `except` or
-- `except_any` attribute to narrow the scope in which the rule is applied.
-- Here's a basic example of a minimal `gears.matcher`.
--
-- @DOC_text_gears_matcher_default_EXAMPLE@
--
-- More examples are available in `awful.rules`.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @see awful.rules
-- @module gears.matcher

local gtable = require("gears.table")
local gsort = require("gears.sort")
local gdebug = require("gears.debug")
local gobject = require("gears.object")
local protected_call = require("gears.protected_call")

local matcher = {}

--- A rule has been added to a set of matching rules.
-- @signal rule::appended
-- @tparam table gears.matcher The matcher.
-- @tparam table rule The rule.
-- @tparam table source The matching rules source name.
-- @tparam table content The matching rules source content.
-- @see append_rule
-- @see append_rules

--- A rule has been removed to a set of matching rules.
-- @signal rule::removed
-- @tparam table gears.matcher The matcher.
-- @tparam table rule The rule.
-- @tparam table source The matching rules source name.
-- @tparam table content The matching rules source content.
-- @see remove_rule

--- A matching source function has been added.
-- @signal matching_function::added
-- @tparam table gears.matcher The matcher.
-- @tparam function callback The callback.
-- @see add_matching_function

--- A matching source table has been added.
-- @signal matching_rules::added
-- @tparam table gears.matcher The matcher.
-- @tparam function callback The callback.
-- @see add_matching_rules

--- A matching source function has been removed.
-- @signal matching_source::removed
-- @tparam table gears.matcher The matcher.
-- @see remove_matching_source

-- Check if an object matches a rule.
-- @param o The object.
-- #tparam table rule The rule to check.
-- @treturn boolean True if it matches, false otherwise.
function matcher:_match(o, rule)
    if not rule then return false end
    for field, value in pairs(rule) do
        local pm = self._private.prop_matchers[field]
        if pm then
            if not pm(o, value, field) then
                return false
            end
        elseif o[field] ~= nil then
            if type(o[field]) == "string" then
                if not o[field]:match(value) and o[field] ~= value then
                    return false
                end
            elseif o[field] ~= value then
                return false
            end
        else
            return false
        end
    end
    return true
end

-- Check if an object matches any part of a rule.
-- @param o The object.
-- #tparam table rule The rule to check.
-- @treturn boolean True if at least one rule is matched, false otherwise.
function matcher:_match_any(o, rule)
    if not rule then return false end
    for field, values in pairs(rule) do
        if o[field] then
            for _, value in ipairs(values) do
                local pm = self._private.prop_matchers[field]
                if pm and pm(o, value, field) then
                    return true
                elseif o[field] == value then
                    return true
                elseif type(o[field]) == "string" and o[field]:match(value) then
                    return true
                end
            end
        end
    end
    return false
end

--- Does a given rule entry match an object?
-- @param o The object.
-- @tparam table entry Rule entry (with keys `rule`, `rule_any`, `except` and/or
--   `except_any`).
-- @treturn boolean If `o` matches `entry`.
-- @method matches_rule
function matcher:matches_rule(o, entry)
    local match = self:_match(o, entry.rule) or self:_match_any(o, entry.rule_any)
    return match
        and (not self:_match(o, entry.except))
        and (not self:_match_any(o, entry.except_any))
end

--- Get list of matching rules for an object.
--
-- If the `rules` argument is not provided, the rules added with
-- `add_matching_rules` will be used.
--
-- @param o The object.
-- @tparam[opt=nil] table rules The rules to check. List with "rule", "rule_any",
--  "except" and "except_any" keys. If no rules are provided, one is selected at
--  random. Unless more rule sources are added, there is only one to begin with.
-- @treturn table The list of matched rules.
-- @method matching_rules
function matcher:matching_rules(o, rules)
    rules = rules or select(2, next(self._matching_rules))

    local result = {}

    if not rules then
        gdebug.print_warning("This matcher has no rule source")
        return result
    end

    for _, entry in ipairs(rules) do
        if self:matches_rule(o, entry) then
            table.insert(result, entry)
        end
    end
    return result
end

--- Check if an object matches a given set of rules.
-- @param o The object.
-- @tparam table rules The rules to check. List of tables with `rule`,
--  `rule_any`, `except` and `except_any` keys.
-- @treturn boolean True if at least one rule is matched, false otherwise.
-- @method matches_rules
function matcher:matches_rules(o, rules)
    for _, entry in ipairs(rules) do
        if self:matches_rule(o, entry) then
            return true
        end
    end
    return false
end

--- Assign a function to match an object property against a value.
--
-- The default matcher uses the `==` operator for all types. It also uses the
-- `:match()` method for string and allows pattern matching. If the value is a
-- function, then that function is called with the object and the current
-- properties to be applied. If the function returns true, the match is
-- accepted.
--
-- Custom property matcher are useful when objects are compared. This avoids
-- having to implement custom metatable for everything.
--
-- The `f` function receives 3 arguments:
--
-- * The object to match against (anything)
-- * The value to compare
-- * The property/field name.
--
-- It should return `true` if it matches and `false` otherwise.
--
-- @tparam string name The property name.
-- @tparam function f The matching function.
-- @method add_property_matcher
-- @usage -- Manually match the screen in various ways.
-- matcher:add_property_matcher("screen", function(c, value)
--    return c.screen == value
--        or screen[c.screen] == value
--        or c.screen.outputs[value] ~= nil
--        or value == "any"
--        or (value == "primary" and c.screen == screen.primary)
-- end)
--
function matcher:add_property_matcher(name, f)
    assert(not self._private.prop_matchers[name], name .. " already has a matcher")

    self._private.prop_matchers[name] = f

    self:emit_signal("property_matcher::added", name, f)
end

--- Add a special setter for a property.
--
-- This is useful to add more properties to object which only make sense within
-- the context of a rule.
--
-- @method add_property_setter
-- @tparam string name The property name.
-- @tparam function f The setter function.
function matcher:add_property_setter(name, f)
    assert(not self._private.prop_setters[name], name .. " already has a matcher")

    self._private.prop_setters[name] = f

    self:emit_signal("property_setter::added", name, f)
end

local function default_rules_callback(self, o, props, callbacks, rules)
    for _, entry in ipairs(self:matching_rules(o, rules)) do
        gtable.crush(props, entry.properties or {})

        if entry.callback then
            table.insert(callbacks, entry.callback)
        end
    end
end

--- Add a set of matching rules.
--
-- @tparam string name The provider name. It must be unique.
-- @tparam table rules A set of rules (see how they work at the top of this
--  page).
-- @tparam[opt={}] table depends_on A list of names of sources this source
--  depends on (sources that must be executed *before* `name`).
-- @tparam[opt={}] table precede A list of names of sources this source has a
--  priority over.
-- @treturn boolean Returns false if a dependency conflict was found.
-- @method add_matching_rules
function matcher:add_matching_rules(name, rules, depends_on, precede)
    local function matching_fct(_self, c, props, callbacks)
        default_rules_callback(_self, c, props, callbacks, rules)
    end

    self._matching_rules[name] = rules

    self:emit_signal("matching_rules::added", rules)

    return self:add_matching_function(name, matching_fct, depends_on, precede)
end

--- Add a matching function.
--
-- @tparam string name The provider name. It must be unique.
-- @tparam function callback The callback that is called to produce properties.
-- @tparam gears.matcher callback.self The matcher object.
-- @param callback.o The object.
-- @tparam table callback.properties The current properties. The callback should
--  add to and overwrite properties in this table.
-- @tparam table callback.callbacks A table of all callbacks scheduled to be
--  executed after the main properties are applied.
-- @tparam[opt={}] table depends_on A list of names of sources this source depends on
--  (sources that must be executed *before* `name`).
-- @tparam[opt={}] table precede A list of names of sources this source has a
--  priority over.
-- @treturn boolean Returns false if a dependency conflict was found.
-- @method add_matching_function
function matcher:add_matching_function(name, callback, depends_on, precede)
    depends_on = depends_on  or {}
    precede    = precede or {}
    assert(type( depends_on ) == "table")
    assert(type( precede    ) == "table")

    for _, v in ipairs(self._matching_source) do
        -- Names must be unique
        assert(
            v.name ~= name,
            "Name must be unique, but '" .. name .. "' was already registered."
        )
    end

    local new_sources = self._rule_source_sort:clone()

    new_sources:prepend(name, precede    )
    new_sources:append (name, depends_on )

    local res, err = new_sources:sort()

    if err then
        gdebug.print_warning("Failed to add the rule source: "..err)
        return false
    end

    -- Only replace the source once the additions has been proven to be safe.
    self._rule_source_sort = new_sources

    local callbacks = {}

    -- Get all callbacks for *existing* sources.
    -- It is important to remember that names can be used in the sorting even
    -- if the source itself doesn't (yet) exist.
    for _, v in ipairs(self._matching_source) do
        callbacks[v.name] = v.callback
    end

    self._matching_source = {}
    callbacks[name] = callback

    for _, v in ipairs(res) do
        if callbacks[v] then
            table.insert(self._matching_source, 1, {
                callback = callbacks[v],
                name     = v
            })
        end
    end

    self:emit_signal("matching_function::added", callback)

    return true
end

--- Remove a source.
--
-- This removes sources added with `add_matching_function` or
-- `add_matching_rules`.
--
-- @tparam string name The source name.
-- @treturn boolean If the source has been removed.
-- @method remove_matching_source
function matcher:remove_matching_source(name)
    self._rule_source_sort:remove(name)

    for k, v in ipairs(self._matching_source) do
        if v.name == name then
            self:emit_signal("matching_source::removed", v)
            table.remove(self._matching_source, k)
            return true
        end
    end


    self._matching_rules[name] = nil

    return false
end

--- Apply awful.rules.rules to an object.
--
-- Calling this will apply all properties provided by the matching functions
-- and rules.
--
-- @param o The object.
-- @method apply
function matcher:apply(o)
    local callbacks, props = {}, {}
    for _, v in ipairs(self._matching_source) do
        v.callback(self, o, props, callbacks)
    end

    self:_execute(o, props, callbacks)
end

-- Execute the rules for the object `o`.
-- @param o The object.
-- @tparam table props A list of properties to apply.
-- @tparam table callbacks A list of callback to execute with the object `o` as
--  sole argument. The callbacks are executed *before* applying the properties.
-- @see gears.matcher.apply
function matcher:_execute(o, props, callbacks)
    -- Apply all callbacks.
    if callbacks then
        for _, callback in pairs(callbacks) do
            protected_call(callback, o)
        end
    end

    for property, value in pairs(props) do
        if type(value) == "function" then
            value = value(o, props)
        end

        if self._private.prop_setters[property] then
            self._private.prop_setters[property](o, value)
        elseif type(o[property]) == "function" then
            o[property](o, value)
        else
            o[property] = value
        end
    end
end

--- Add a new rule to the default set.
-- @tparam string source The source name.
-- @tparam table rule A valid rule.
-- @method append_rule
function matcher:append_rule(source, rule)
    if not self._matching_rules[source] then
        self:add_matching_rules(source, {}, {}, {})
    end
    table.insert(self._matching_rules[source], rule)
    self:emit_signal("rule::appended", rule, source, self._matching_rules[source])
end

--- Add a new rules to the default set.
-- @tparam string source The source name.
-- @tparam table rules A table with rules.
-- @method append_rules
function matcher:append_rules(source, rules)
    for _, rule in ipairs(rules) do
        self:append_rule(source, rule)
    end
end

--- Remove a new rule to the default set.
-- @tparam string source The source name.
-- @tparam string|table rule An existing rule or its `id`.
-- @treturn boolean If the rule was removed.
-- @method remove_rule
function matcher:remove_rule(source, rule)
    if not self._matching_rules[source] then return end

    for k, v in ipairs(self._matching_rules[source]) do
        if v == rule or v.id == rule then
            table.remove(self._matching_rules[source], k)
            self:emit_signal("rule::removed", rule, source, self._matching_rules[source])
            return true
        end
    end

    return false
end

local module = {}

--- Create a new rule solver object.
-- @constructorfct gears.matcher
-- @return A new rule solver object.

local function new()
    local ret = gobject()

    rawset(ret, "_private", {
        rules = {}, prop_matchers = {}, prop_setters = {}
    })

    -- Contains the sources.
    -- The elements are ordered "first in, first executed". Thus, the higher the
    -- index, the higher the priority. Each entry is a table with a `name` and a
    -- `callback` field. This table is exposed for debugging purpose. The API
    -- is private and should only be modified using the public accessors.
    ret._matching_source = {}
    ret._rule_source_sort = gsort.topological()
    ret._matching_rules = {}

    gtable.crush(ret, matcher, true)

    return ret
end

--@DOC_rule_COMMON@

--@DOC_object_COMMON@

return setmetatable(module, {__call = new})
