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

end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
