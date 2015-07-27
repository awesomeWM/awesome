---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
---------------------------------------------------------------------------

local cache = require("gears.cache")

describe("gears.cache", function()
    -- Make sure no cache is cleared during the tests
    before_each(function()
        collectgarbage("stop")
    end)
    after_each(function()
        collectgarbage("restart")
    end)

    describe("Zero arguments", function()
        it("Creation cb is called", function()
            local called = false
            local c = cache(function()
                called = true
            end)
            local res = c:get()
            assert.is_nil(res)
            assert.is_true(called)
        end)
    end)

    describe("Two arguments", function()
        it("Cache works", function()
            local num_calls = 0
            local c = cache(function(a, b)
                num_calls = num_calls + 1
                return a + b
            end)
            local res1 = c:get(1, 2)
            local res2 = c:get(1, 3)
            local res3 = c:get(1, 2)
            assert.is.equal(res1, 3)
            assert.is.equal(res2, 4)
            assert.is.equal(res3, 3)
            assert.is.equal(num_calls, 2)
        end)

        it("Cache invalidation works", function()
            local num_calls = 0
            local c = cache(function(a, b)
                num_calls = num_calls + 1
                return a + b
            end)
            local res1 = c:get(1, 2)
            collectgarbage("collect")
            local res2 = c:get(1, 2)
            assert.is.equal(res1, 3)
            assert.is.equal(res2, 3)
            assert.is.equal(num_calls, 2)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
