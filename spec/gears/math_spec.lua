
local gmath = require("gears.math")


describe("gears.math", function()
    it("math.cycle.basic", function()
        for size=5, 11 do
            assert.is.same(gmath.cycle(size,  1, true), 1)
            assert.is.same(gmath.cycle(size,  2, true), 2)
            assert.is.same(gmath.cycle(size,  0, true), 10)
            assert.is.same(gmath.cycle(size, -1, true), size-1)
            assert.is.same(gmath.cycle(size, -2, true), size-2)
        end
    end)

    it("math.cycle.sequential", function()
        local t = { "a", 1, function() end, false}
        for size=5, 11 do
            local prev = nil
            for i = -12, 12 do
                local cur = gmath.cycle(size, i, true)
                assert.is_true(cur > 0 and cur <= size)
                assert.is_true((not prev) or cur - prev == 1)
            end
    end)
end)
