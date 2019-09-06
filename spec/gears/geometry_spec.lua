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

    describe("rectangle.get_in_direction", function()
        local dir_rects = {
            { x = 0, y = 0, width = 1, height = 1 },
            { x = 1, y = 0, width = 1, height = 1 },
            { x = 0, y = 1, width = 1, height = 1 },
            { x = 1, y = 1, width = 1, height = 1 },
        }

        it("left", function()
            assert.is.equal(1, geo.rectangle.get_in_direction("left", dir_rects, dir_rects[2]))
        end)

        it("right", function()
            assert.is.equal(2, geo.rectangle.get_in_direction("right", dir_rects, dir_rects[1]))
        end)

        it("top", function()
            assert.is.equal(2, geo.rectangle.get_in_direction("up", dir_rects, dir_rects[4]))
        end)

        it("bottom", function()
            assert.is.equal(4, geo.rectangle.get_in_direction("down", dir_rects, dir_rects[2]))
        end)

        it("gets no screens if none exist in the direction", function()
            assert.is_nil(geo.rectangle.get_in_direction("up", dir_rects, dir_rects[2]))
            assert.is_nil(geo.rectangle.get_in_direction("down", dir_rects, dir_rects[4]))
            assert.is_nil(geo.rectangle.get_in_direction("right", dir_rects, dir_rects[2]))
            assert.is_nil(geo.rectangle.get_in_direction("left", dir_rects, dir_rects[1]))
        end)
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

    describe("rectangle.are_equal", function()

        it("with equality", function()
            assert.are_equal(true, geo.rectangle.are_equal(
                {x=0, y=0, width=10, height=10},
                {x=0, y=0, width=10, height=10}
            ))
        end)

        it("without equality", function()
            assert.are_equal(false, geo.rectangle.are_equal(
                {x=0, y=0, width=1, height=1},
                {x=2, y=2, width=1, height=1}
            ))
        end)

        it("with intersection", function()
            assert.are_equal(false, geo.rectangle.are_equal(
                {x=0, y=0, width=1, height=1},
                {x=0, y=0, width=2, height=2}
            ))
        end)
    end)

    describe("rectangle.area_remove", function()
        -- TODO perhaps it would be better to compare against a cairo.region
        -- than to have this overly specific tests?

        local function test(expected, areas, elem)
            local result = geo.rectangle.area_remove(areas, elem)
            assert.is.equal(result, areas)
            assert.is.same(expected, areas)
        end

        it("non-intersecting", function()
            local areas = {{ x = 0, y = 0, width = 100, height = 100 }}
            local elem = { x = -50, y = -50, width = 25, height = 25}
            local expected = {{ x = 0, y = 0, width = 100, height = 100 }}
            test(expected, areas, elem)
        end)

        it("center", function()
            local areas = {{ x = 0, y = 0, width = 100, height = 100 }}
            local elem = { x = 25, y = 25, width = 50, height = 50}
            local expected = {
                { x = 0, y = 0, width = 25, height = 100 },
                { x = 0, y = 0, width = 100, height = 25 },
                { x = 75, y = 0, width = 25, height = 100 },
                { x = 0, y = 75, width = 100, height = 25 },
            }
            test(expected, areas, elem)
        end)

        it("top", function()
            local areas = {{ x = 0, y = 0, width = 100, height = 100 }}
            local elem = { x = 25, y = 0, width = 50, height = 50}
            local expected = {
                { x = 0, y = 0, width = 25, height = 100 },
                { x = 75, y = 0, width = 25, height = 100 },
                { x = 0, y = 50, width = 100, height = 50 },
            }
            test(expected, areas, elem)
        end)

        it("bottom", function()
            local areas = {{ x = 0, y = 0, width = 100, height = 100 }}
            local elem = { x = 25, y = 50, width = 50, height = 50}
            local expected = {
                { x = 0, y = 0, width = 25, height = 100 },
                { x = 0, y = 0, width = 100, height = 50 },
                { x = 75, y = 0, width = 25, height = 100 },
            }
            test(expected, areas, elem)
        end)

        it("left", function()
            local areas = {{ x = 0, y = 0, width = 100, height = 100 }}
            local elem = { x = 0, y = 25, width = 50, height = 50}
            local expected = {
                { x = 0, y = 0, width = 100, height = 25 },
                { x = 50, y = 0, width = 50, height = 100 },
                { x = 0, y = 75, width = 100, height = 25 },
            }
            test(expected, areas, elem)
        end)

        it("right", function()
            local areas = {{ x = 0, y = 0, width = 100, height = 100 }}
            local elem = { x = 50, y = 25, width = 50, height = 50}
            local expected = {
                { x = 0, y = 0, width = 50, height = 100 },
                { x = 0, y = 0, width = 100, height = 25 },
                { x = 0, y = 75, width = 100, height = 25 },
            }
            test(expected, areas, elem)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
