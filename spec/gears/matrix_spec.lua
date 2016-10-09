---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
---------------------------------------------------------------------------

local matrix = require("gears.matrix")
local cairo = require("lgi").cairo

describe("gears.matrix", function()
    local function round(n)
        return math.floor(n + 0.5)
    end

    describe("cannot modify", function()
        assert.has.errors(function()
            matrix.identity.something = 42
        end)
    end)

    describe("equals", function()
        it("Same matrix", function()
            local m = matrix.create_rotate(1)
            assert.is_true(matrix.equals(m, m))
            assert.is.equal(m, m)
        end)

        it("Different matrix equals", function()
            local m1 = matrix.create_rotate(1)
            local m2 = matrix.create_rotate(1)
            assert.is_true(matrix.equals(m1, m2))
            assert.is.equal(m1, m2)
        end)

        it("Different matrix unequal", function()
            local m1 = matrix.create(1, 2, 3, 4, 5, 6)
            local m2 = matrix.create(1, 2, 3, 4, 5, 0)
            assert.is_false(matrix.equals(m1, m2))
            assert.is_not.equal(m1, m2)
        end)

        it("Identity matrix", function()
            local m1 = matrix.identity
            local m2 = matrix.create(1, 0, 0, 1, 0, 0)
            assert.is_true(matrix.equals(m1, m2))
            assert.is.equal(m1, m2)
        end)
    end)

    describe("create", function()
        it("translate", function()
            assert.is.equal(matrix.create(1, 0, 0, 1, 2, 3), matrix.create_translate(2, 3))
        end)

        it("scale", function()
            assert.is.equal(matrix.create(2, 0, 0, 3, 0, 0), matrix.create_scale(2, 3))
        end)

        it("rotate", function()
            local m = matrix.create_rotate(math.pi / 2)
            assert.is_true(math.abs(round(m.xx) - m.xx) < 0.00000001)
            assert.is.equal(-1, m.xy)
            assert.is.equal(1, m.yx)
            assert.is_true(math.abs(round(m.yy) - m.yy) < 0.00000001)
            assert.is.equal(0, m.x0)
            assert.is.equal(0, m.y0)
        end)
    end)

    it("multiply", function()
        -- Just some random matrices which I multiplied by hand
        local a = matrix.create(1, 2, 3, 4, 5, 6)
        local b = matrix.create(7, 8, 9, 1, 1, 1)
        local m = matrix.create(25, 10, 57, 28, 90, 47)
        assert.is.equal(m, a:multiply(b))
        assert.is.equal(m, a * b)
    end)

    describe("invert", function()
        it("translate", function()
            local m1, m2 = matrix.create_translate(2, 3), matrix.create_translate(-2, -3)
            assert.is.equal(m2, m1:invert())
            assert.is.equal(matrix.identity, m1 * m2)
            assert.is.equal(matrix.identity, m2 * m1)
        end)

        it("scale", function()
            local m1, m2 = matrix.create_scale(2, 3), matrix.create_scale(1/2, 1/3)
            assert.is.equal(m2, m1:invert())
            assert.is.equal(matrix.identity, m1 * m2)
            assert.is.equal(matrix.identity, m2 * m1)
        end)

        it("rotate", function()
            local m1, m2 = matrix.create_rotate(2), matrix.create_rotate(-2)
            assert.is.equal(m2, m1:invert())
            assert.is.equal(matrix.identity, m1 * m2)
            assert.is.equal(matrix.identity, m2 * m1)
        end)
    end)

    describe("transform_rectangle", function()
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
            test(matrix.identity, 1, 2, 3, 4, 1, 2, 3, 4)
        end)

        it("Rotate 180", function()
            test(matrix.create_rotate(math.pi),
                1, 2, 3, 4, -4, -6, 3, 4)
        end)

        it("Rotate 90", function()
            test(matrix.create_rotate(math.pi / 2),
                1, 2, 3, 4, -6, 1, 4, 3)
        end)
    end)

    it("tostring", function()
        local m = matrix.create(1, 2, 3, 4, 5, 6)
        local expected = "[[1, 2], [3, 4], [5, 6]]"
        assert.is.equal(expected, m:tostring())
        assert.is.equal(expected, tostring(m))
    end)

    it("from_cairo_matrix", function()
        local m1 = matrix.create_translate(2, 3)
        local m2 = matrix.from_cairo_matrix(cairo.Matrix.create_translate(2, 3))
        assert.is.equal(m1, m2)
    end)

    it("to_cairo_matrix", function()
        local m = matrix.create_scale(3, 4):to_cairo_matrix()
        assert.is.equal(3, m.xx)
        assert.is.equal(0, m.xy)
        assert.is.equal(0, m.yx)
        assert.is.equal(4, m.yy)
        assert.is.equal(0, m.x0)
        assert.is.equal(0, m.y0)
    end)

    describe("rotate_at", function()
        it("create", function()
            local m = matrix.create_rotate_at(5, 5, math.pi)
            assert.is.equal(-1, m.xx)
            -- Stupid rounding issues...
            assert.is_true(math.abs(round(m.xy) - m.xy) < 0.00000001)
            assert.is_true(math.abs(round(m.yx) - m.yx) < 0.00000001)
            assert.is.equal(-1, m.yy)
            assert.is.equal(10, m.x0)
            assert.is.equal(10, m.y0)
        end)

        it("multiply", function()
            local m = matrix.create_translate(-5, 0):rotate_at(5, 5, math.pi)
            assert.is.equal(-1, m.xx)
            -- Stupid rounding issues...
            assert.is_true(math.abs(round(m.xy) - m.xy) < 0.00000001)
            assert.is_true(math.abs(round(m.yx) - m.yx) < 0.00000001)
            assert.is.equal(-1, m.yy)
            assert.is.equal(15, m.x0)
            assert.is.equal(10, m.y0)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
