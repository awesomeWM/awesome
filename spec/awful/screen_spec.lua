---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2016 Uli Schlachter
---------------------------------------------------------------------------

local fake_screens = {}

_G.screen = setmetatable({
    add_signal = function() end,
    set_index_miss_handler = function() end,
    set_newindex_miss_handler = function() end,
}, {
    __call = function(_, _, prev)
        if not prev then
            return fake_screens[1]
        end
        for k, v in ipairs(fake_screens) do
            if v == prev then
                return fake_screens[k+1]
            end
        end
    end,

    __index = function(_, arg)
        if arg == "primary" then
            return fake_screens[1]
        end
        if type(arg) == "number" then
            return fake_screens[arg]
        end
        for _, v in ipairs(fake_screens) do
            if v == arg then
                return arg
            end
        end
    end,

    __newindex = error,
})
local ascreen = require("awful.screen")

describe("awful.screen", function()
    describe("two screens each 1x1", function()
        before_each(function()
            fake_screens = {
                { geometry = { x = 0, y = 0, width = 1, height = 1 }, index = 1 },
                { geometry = { x = 0, y = 1, width = 1, height = 1 }, index = 2 },
            }
        end)

        it("get_square_distance", function()
            assert.is.equal(0, ascreen.object.get_square_distance(fake_screens[1], 0, 0))
            assert.is.equal(1, ascreen.object.get_square_distance(fake_screens[1], 0, 1))
            assert.is.equal(2, ascreen.object.get_square_distance(fake_screens[1], 1, 1))
            assert.is.equal(5, ascreen.object.get_square_distance(fake_screens[1], 2, 1))
            assert.is.equal(2, ascreen.object.get_square_distance(fake_screens[1], -1, 1))

            assert.is.equal(1, ascreen.object.get_square_distance(fake_screens[2], 0, 0))
            assert.is.equal(0, ascreen.object.get_square_distance(fake_screens[2], 0, 1))
            assert.is.equal(1, ascreen.object.get_square_distance(fake_screens[2], 1, 1))
            assert.is.equal(4, ascreen.object.get_square_distance(fake_screens[2], 2, 1))
            assert.is.equal(1, ascreen.object.get_square_distance(fake_screens[2], -1, 1))
        end)

        it("getbycoord", function()
            -- Some exact matches
            assert.is.equal(1, ascreen.getbycoord(0, 0))
            assert.is.equal(2, ascreen.getbycoord(0, 1))

            -- Some "find the closest screen"-cases
            assert.is.equal(1, ascreen.getbycoord(-1, -1))
            assert.is.equal(1, ascreen.getbycoord(-1, 0))
            assert.is.equal(2, ascreen.getbycoord(-1, 1))
            assert.is.equal(2, ascreen.getbycoord(-1, 2))

            assert.is.equal(2, ascreen.getbycoord(1, 1))
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
