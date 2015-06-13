---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
---------------------------------------------------------------------------

local matrix = require("gears.matrix")
local cairo = require("lgi").cairo

describe("gears.matrix", function()
    describe("copy", function()
        it("Test copy", function()
            local m1 = cairo.Matrix()
            m1:init(1, 2, 3, 4, 5, 6)
            local m2 = matrix.copy(m1)
            assert.is.equal(m1.xx, m2.xx)
            assert.is.equal(m1.xy, m2.xy)
            assert.is.equal(m1.yx, m2.yx)
            assert.is.equal(m1.yy, m2.yy)
            assert.is.equal(m1.x0, m2.x0)
            assert.is.equal(m1.y0, m2.y0)

            m1.x0 = 42
            assert.is_not.equal(m1.x0, m2.x0)
        end)
    end)

    describe("equals", function()
        it("Same matrix", function()
            local m = cairo.Matrix.create_rotate(1)
            assert.is_true(matrix.equals(m, m))
        end)

        it("Different matrix equals", function()
            local m1 = cairo.Matrix.create_rotate(1)
            local m2 = cairo.Matrix.create_rotate(1)
            assert.is_true(matrix.equals(m1, m2))
        end)

        it("Different matrix unequal", function()
            local m1 = cairo.Matrix()
            local m2 = cairo.Matrix()
            m1:init(1, 2, 3, 4, 5, 6)
            m2:init(1, 2, 3, 4, 5, 0)
            assert.is_false(matrix.equals(m1, m2))
        end)
    end)

    describe("transform_rectangle", function()
        local function round(n)
            return math.floor(n + 0.5)
        end
        local function test(m, x, y, width, height,
            expected_x, expected_y, expected_width, expected_height)
            local actual_x, actual_y, actual_width, actual_height =
                matrix.transform_rectangle(m, x, y, width, height)
            assert.is.equal(round(actual_x), expected_x)
            assert.is.equal(round(actual_y), expected_y)
            assert.is.equal(round(actual_width), expected_width)
            assert.is.equal(round(actual_height), expected_height)
            -- Stupid rounding issues...
            assert.is_true(math.abs(round(actual_x) - actual_x) < 0.00000001)
            assert.is_true(math.abs(round(actual_y) - actual_y) < 0.00000001)
            assert.is_true(math.abs(round(actual_width) - actual_width) < 0.00000001)
            assert.is_true(math.abs(round(actual_height) - actual_height) < 0.00000001)
        end
        it("Identity matrix", function()
            test(cairo.Matrix.create_identity(), 1, 2, 3, 4, 1, 2, 3, 4)
        end)

        it("Rotate 180", function()
            test(cairo.Matrix.create_rotate(math.pi),
                1, 2, 3, 4, -4, -6, 3, 4)
        end)

        it("Rotate 90", function()
            test(cairo.Matrix.create_rotate(math.pi / 2),
                1, 2, 3, 4, -6, 1, 4, 3)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
