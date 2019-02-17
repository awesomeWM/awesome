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
            answer = 42,
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

    -- Apply the properties to `o`
    matcher:apply(o)

assert(o.is_everything) --DOC_HIDE
assert(o.name == "baz") --DOC_HIDE
matcher:remove_matching_source("first") --DOC_HIDE
matcher:remove_matching_source("second") --DOC_HIDE
