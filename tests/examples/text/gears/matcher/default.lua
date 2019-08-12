--DOC_HIDE --DOC_NO_USAGE --DOC_GEN_OUTPUT
local gears = {matcher = require("gears.matcher")} --DOC_HIDE

    local o = {
        foo    = "bar",
        answer = 42,
    }

--DOC_NEWLINE

    -- This rule will match
    local rule1 = {
        rule = {
            answer     = 42,
            everything = true,
        },
        properties = {
            name = "baz",
        },
    }

--DOC_NEWLINE

    -- This rule will **not** match
    local rule2 = {
        -- When the rule properties are strings, the Lua
        --pattern matching is used.
        rule = {
            foo = "[f]+",
        },
        properties = {
            name = "foobar",
        },
    }

--DOC_NEWLINE

    local rules = {
        rule1,
        rule2,
    }

--DOC_NEWLINE

    local matcher = gears.matcher()

--DOC_NEWLINE

    local function first_source(self, object, props, callbacks) --luacheck: no unused args
        assert(self:matching_rules(object, rules)[1] == rule1) --DOC_HIDE
        assert(#self:matching_rules(object, rules) == 1) --DOC_HIDE
        assert(self:matches_rule(object, rule1)) --DOC_HIDE
        assert(not self:matches_rule(object, rule2)) --DOC_HIDE

        -- In this callback, you can add new elements to the props and
        -- callbacks tables. It is not recommended the modify `object` in
        -- this callback.
        if object.answer == 42 then
            props.is_everything = true
        end
    end

--DOC_NEWLINE

    -- This will add a custom function to add properties to the rules.
    matcher:add_matching_function("first", first_source, {}, {})

--DOC_NEWLINE

    -- This will add the `rules` to this matcher.
    matcher:add_matching_rules("second", rules, {"first"}, {})

--DOC_NEWLINE
    -- Some properties cannot be checked with the `==` operator (like those
    -- with multiple possible types). In that case, it is possible to define
    -- special comparator function.
    matcher:add_property_matcher("everything", function(obj, value)
        return value and obj.answer == 42
    end)

--DOC_NEWLINE

    -- The same can be done for the property section.
    matcher:add_property_setter("multiply_by", function(obj, value)
        obj.answer = (obj.answer or 1) * value
    end)

--DOC_NEWLINE

    -- It is possible to append rules to existing (or new) sources.
    matcher:append_rule( "second", {
        id   = "rule_with_id",
        rule = {
            has_elite = true,
        },
        properties = {
            multiply_by = "1337",
        },
    })

--DOC_NEWLINE

    -- Or remove them.
    local rm3 = --DOC_HIDE
    matcher:remove_rule("second", "rule_with_id")
    assert(rm3) --DOC_HIDE

--DOC_NEWLINE

    -- Apply the properties to `o`
    matcher:apply(o)

assert(o.is_everything) --DOC_HIDE
assert(o.name == "baz") --DOC_HIDE
local rm1 = --DOC_HIDE
matcher:remove_matching_source("first") --DOC_HIDE
assert(rm1) --DOC_HIDE

matcher:append_rules("second", {{},{},{}}) --DOC_HIDE

local rm2 = --DOC_HIDE
matcher:remove_matching_source("second") --DOC_HIDE
assert(rm2) --DOC_HIDE
