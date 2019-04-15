--DOC_HIDE
local gears = {matcher = require("gears.matcher")} --DOC_HIDE

local o = { --DOC_HIDE
    foo    = "bar", --DOC_HIDE
    answer = 42, --DOC_HIDE
} --DOC_HIDE

local rules = {
    {
        rule = {
            answer = 42,
        },
        properties = {
            name = "baz",
        },
    }
}

--DOC_NEWLINE

local matcher = gears.matcher()--DOC_HIDE
matcher:add_matching_rules("second", rules, {"first"}, {}) --DOC_HIDE
matcher:apply(o) --DOC_HIDE
