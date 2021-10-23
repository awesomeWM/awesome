---------------------------------------------------------------------------
-- @author Yauheni Kirylau
-- @copyright 2020 Yauheni Kirylau
---------------------------------------------------------------------------

local matcher = require("gears.matcher")

local matcher_instance = matcher()
local test_obj = {
    foo='bar',
    spam='',
}

describe("gears.matcher", function()

    describe("matching by normal string value", function()
        local rule = {
            foo='bar',
        }
        assert.is_true(matcher_instance:_match(test_obj, rule))
    end)

    describe("not matching by normal string value", function()
        local rule = {
            foo='nah',
        }
        assert.is_false(matcher_instance:_match(test_obj, rule))
    end)

    describe("matching by empty string value", function()
        local rule = {
            spam='',
        }
        assert.is_true(matcher_instance:_match(test_obj, rule))
    end)

    describe("not matching by empty string value", function()
        local rule = {
            foo='',
        }
        assert.is_false(matcher_instance:_match(test_obj, rule))
    end)

    describe("match_any positive", function()
        local rule = {
            foo={''},
            spam={''},
        }
        assert.is_true(matcher_instance:_match_any(test_obj, rule))
    end)

    describe("match_any negative", function()
        local rule = {
            foo={''},
            spam={'bar'},
        }
        assert.is_false(matcher_instance:_match_any(test_obj, rule))
    end)

    describe("match_every full negative", function()
        local rule = {
            foo={''},
            spam={'bar'},
        }
        assert.is_false(matcher_instance:_match_every(test_obj, rule))
    end)

    describe("match_every partial negative", function()
        local rule = {
            foo={''},
            spam={''},
        }
        assert.is_false(matcher_instance:_match_every(test_obj, rule))
    end)

    describe("match_every positive", function()
        local rule = {
            foo={'bar'},
            spam={''},
        }
        assert.is_true(matcher_instance:_match_every(test_obj, rule))
    end)

     describe("check main vs. fallback rules", function()
        local m = matcher()

        local rules = {
            {
                id       = "main",
                fallback = false,
                rule     = {
                    foo = "bar"
                },
                properties = {
                    main_applied = true,
                }
            },
            {
                id       = "fallback",
                fallback = true,
                rule     = {
                    everything = 42
                },
                properties = {
                    fallback_applied = true,
                }
            },
        }

        m:append_rules("default", rules)

        -- Matches `main` and `fallback`
        local obj1 = {
            foo        = "bar",
            everything = 42,
        }

        -- Only matches `fallback`.
        local obj2 = {
            foo        = "baz",
            everything = 42,
        }

        m:apply(obj1)
        m:apply(obj2)

        assert.is_true(m:_match(obj1, rules[1]["rule"]))
        assert.is_true(m:_match(obj1, rules[2]["rule"]))
        assert.is_true(m:_match(obj2, rules[2]["rule"]))
        assert.is_false(m:_match(obj2, rules[1]["rule"]))

        assert.is_true(obj1.main_applied)
        assert.is_true(obj2.fallback_applied)
        assert.is_nil(obj2.main_applied)
        assert.is_nil(obj1.fallback_applied)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
