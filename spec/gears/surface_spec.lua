local surface = require("gears.surface")
local color = require("gears.color")
local lgi = require("lgi")
local cairo = lgi.cairo
local gdk = lgi.Gdk -- lgi.require("Gdk", "3")
--local pixbuf = lgi.GdkPixbuf

describe("gears.surface", function ()
    local function test_square()
        local surf = cairo.ImageSurface(cairo.Format.ARGB32, 50, 50)
        local ctx = cairo.Context(surf)
        ctx:rectangle(0,0,50,50)
        local pattern = cairo.Pattern.create_linear(0, 0, 50, 0)
        pattern:add_color_stop_rgba(0, color.parse_color("#000000"))
        pattern:add_color_stop_rgba(1, color.parse_color("#FFFFFF"))
        ctx:set_source(pattern)
        ctx:fill()

        return surf
    end
    describe("crop_surface_by_ratio", function ()
        ---@param ratio number target img ratio
        ---@param target_width number target img width the crop is tested against
        ---@param target_heigth number target img height the crop is tested against
        local function test(ratio, target_width, target_heigth)
            local surf = test_square()
            local out = surface.crop_surface_by_ratio(surf, ratio)
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
    describe("crop_surface", function ()
        --TODO: Find a way to check for pixel colors to make sure th cutoff is
        -- at the right spot
        ---@param args table
        local function test(args, target_width, target_heigth)
            -- extend args supplied for left right top and bottom
            args.surface = test_square()
            local out = surface.crop_surface(args)
            local w, h = surface.get_size(out)
            assert.is_equal(w, target_width)
            assert.is_equal(h, target_heigth)

            -- pick 0,0 and w,h spot in cropped img and check color values
            local pbuf1_in = gdk.pixbuf_get_from_surface(args.surface, args.left or 0, args.top or 0, 1, 1)
            local pbuf1_out = gdk.pixbuf_get_from_surface(out, 0, 0, 1, 1)
            assert.is_equal(pbuf1_in:get_pixels(), pbuf1_out:get_pixels())
            local pbuf2_in = gdk.pixbuf_get_from_surface(args.surface, (args.left or 0) + w - 1, (args.top or 0) + h - 1, 1, 1)
            local pbuf2_out = gdk.pixbuf_get_from_surface(out, w-1, h-1, 1, 1)
            assert.is_equal(pbuf2_in:get_pixels(), pbuf2_out:get_pixels())
        end

        it("keep size", function ()
            test({}, 50, 50)
        end)

        it("to 50x25", function ()
            test({top = 12, bottom = 13}, 50, 25)
        end)

        it("to 25x50", function ()
            test({left = 12, right = 13}, 25, 50)
        end)

        it("test 1x50", function ()
            test({left = 20, right = 29}, 1, 50)
        end)

        it("test 50x1", function ()
            test({top = 23, bottom = 26}, 50, 1)
        end)

        it("crop all edges", function ()
            test({left = 7, right = 13, top = 9, bottom = 16}, 30, 25)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
