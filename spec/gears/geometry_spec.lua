---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local geo = require("gears.geometry")

describe("gears.geometry", function()
    local rects = {
        { x = 0, y = 0, width = 1920, height = 1080 },
        { x = 1920, y = 0, width = 1280, height = 1024 },
        { x = 3200, y = 0, width = 1920, height = 1080 },
    }

    describe("rectangle.get_square_distance", function()
        it("size 1x1 rectangle", function()
            local rect = { x = 4, y = 7, width = 1, height = 1 }
            assert.is.equal(0, geo.rectangle.get_square_distance(rect, 4, 7))

            assert.is.equal(1, geo.rectangle.get_square_distance(rect, 5, 7))
            assert.is.equal(1, geo.rectangle.get_square_distance(rect, 3, 7))
            assert.is.equal(1, geo.rectangle.get_square_distance(rect, 4, 6))
            assert.is.equal(1, geo.rectangle.get_square_distance(rect, 4, 8))

            assert.is.equal(9, geo.rectangle.get_square_distance(rect, 1, 7))
            assert.is.equal(9, geo.rectangle.get_square_distance(rect, 7, 7))
            assert.is.equal(8, geo.rectangle.get_square_distance(rect, 2, 5))
            assert.is.equal(8, geo.rectangle.get_square_distance(rect, 6, 9))
        end)

        it("real rectangle", function()
            local rect = { x = 10, y = 11, width = 12, height = 13 }
            assert.is.equal(0, geo.rectangle.get_square_distance(rect, 15, 15))
            assert.is.equal(0, geo.rectangle.get_square_distance(rect, 10, 11))
            assert.is.equal(0, geo.rectangle.get_square_distance(rect, 21, 11))
            assert.is.equal(0, geo.rectangle.get_square_distance(rect, 21, 23))
            assert.is.equal(0, geo.rectangle.get_square_distance(rect, 10, 23))
        end)
    end)

    describe("rectangle.get_closest_by_coord", function()
        it("contained", function()
            assert.is.equal(2, geo.rectangle.get_closest_by_coord({
                { x = 0, y = 0, width = 1, height = 1 },
                { x = 3, y = 3, width = 5, height = 5 },
                { x = 10, y = 10, width = 1, height = 1 },
            }, 5, 5))
        end)

        it("outside", function()
            assert.is.equal(3, geo.rectangle.get_closest_by_coord(rects, 314159, 314159))
            assert.is.equal(1, geo.rectangle.get_closest_by_coord(rects, -42, 0))
            assert.is.equal(2, geo.rectangle.get_closest_by_coord(rects, 2000, 1030))
        end)
    end)

    it("rectangle.get_by_coord", function()
        assert.is_nil(geo.rectangle.get_by_coord(rects, 314159, 314159))
        assert.is_nil(geo.rectangle.get_by_coord(rects, -42, 0))
        assert.is_nil(geo.rectangle.get_by_coord(rects, 2000, 1030))
        assert.is.equal(1, geo.rectangle.get_by_coord(rects, 0, 0))
        assert.is.equal(1, geo.rectangle.get_by_coord(rects, 1919, 1079))
        assert.is.equal(2, geo.rectangle.get_by_coord(rects, 2000, 42))
        assert.is.equal(3, geo.rectangle.get_by_coord(rects, 4000, 1050))
    end)

    it("rectangle.get_in_direction", function()
        pending("Write tests for gears.geometry.rectangle.get_in_direction")
    end)

    describe("rectangle.get_intersection", function()
        it("with intersection", function()
            assert.is_same({ x = 10, y = 20, width = 30, height = 30 },
                geo.rectangle.get_intersection(
                    { x = 10, y = 15, width = 100, height = 35 },
                    { x = 0,  y = 20, width = 40,  height = 100 }))
        end)

        it("without intersection horizontal", function()
            local inter = geo.rectangle.get_intersection(
                { x = -20, y = -30, width = 15, height = 25 },
                { x =  20, y = -30, width = 15, height = 25 })
            assert.is_equal(inter.width, 0)
            assert.is_equal(inter.height, 0)
        end)

        it("without intersection vertical", function()
            local inter = geo.rectangle.get_intersection(
                { x = -20, y = -30, width = 15, height = 25 },
                { x = -20, y =  30, width = 15, height = 25 })
            assert.is_equal(inter.width, 0)
            assert.is_equal(inter.height, 0)
        end)
    end)

    it("rectangle.area_remove", function()
        pending("Write tests for gears.geometry.rectangle.area_remove")
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
