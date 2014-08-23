---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local sort = require("gears.sort")

describe("gears.sort", function()
    local function test(expected, input)
        sort.sort(input)
        assert.are.same(expected, input)
    end

    it("with empty input", function()
        test({}, {})
    end)

    it("with sorted input", function()
        test({1, 2, 3, 4, 5, 6},
             {1, 2, 3, 4, 5, 6})
    end)

    it("with reversed input", function()
        test({1, 2, 3, 4, 5, 6},
             {6, 5, 4, 3, 2, 1})
    end)

    it("with repeating items", function()
        test({1, 2, 2, 3, 4, 4, 5},
             {1, 2, 2, 3, 4, 4, 5})
    end)

    it("with repeating items and reversed input", function()
        test({1, 2, 2, 3, 4, 4, 5},
             {5, 4, 4, 3, 2, 2, 1})
    end)

    it("with non-integer keys", function()
        test({2, 3, 4, Awesome = 42},
             {3, 4, 2, Awesome = 42})
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
