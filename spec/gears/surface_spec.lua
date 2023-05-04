local surface = require("gears.surface")
local cairo = require("lgi").cairo

describe("gears.surface", function ()
    describe("crop_surface", function ()
        --- create a 50x50 surface and perform crops on it
        ---@param ratio number target img ratio
        ---@param target_width number target img width the crop is tested against
        ---@param target_heigth number target img height the crop is tested against
        local function test(ratio, target_width, target_heigth)
            local square = cairo.ImageSurface(cairo.Format.ARGB32, 50, 50)
            local out = surface.crop_surface(square, ratio)
            local w, h = surface.get_size(out)
            assert.is_equal(w, target_width)
            assert.is_equal(h, target_heigth)
        end

        it("keep size", function ()
            test(1, 50, 50)
        end)

        it("to 50x25", function ()
            test(2, 50, 25)
        end)

        it("to 25x50", function ()
            test(0.5, 25, 50)
        end)

        it("test minwidth 1x50", function ()
            test(0.001, 1, 50)
        end)

        it("test minheight 50x1", function ()
            test(1000, 50, 1)
        end)

        it("weird calc edgecase 30x50", function ()
            test(3/5, 30, 50)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
