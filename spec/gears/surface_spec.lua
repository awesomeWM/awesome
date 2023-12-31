local surface = require("gears.surface")
local color = require("gears.color")
local lgi = require("lgi")
local cairo = lgi.cairo
local gdk = lgi.Gdk

describe("gears.surface", function ()
    describe("crop_surface", function ()
        local function test_square()
            local surf = cairo.ImageSurface(cairo.Format.ARGB32, 50, 50)
            local ctx = cairo.Context(surf)
            -- creates complicated pattern, to make sure each pixel has a
            -- different color value
            ctx:rectangle(0,0,50,50)
            local pattern = cairo.Pattern.create_linear(0, 0, 50, 50)
            pattern:add_color_stop_rgba(0, color.parse_color("#00000000"))
            pattern:add_color_stop_rgba(1, color.parse_color("#FF00FF55"))
            ctx:set_source(pattern)
            ctx:fill()
            local ctx2 = cairo.Context(surf)
            ctx2:rectangle(0,0,50,50)
            local pattern2 = cairo.Pattern.create_linear(0, 50, 50, 0)
            pattern2:add_color_stop_rgba(0, color.parse_color("#00FF0088"))
            pattern2:add_color_stop_rgba(1, color.parse_color("#00000000"))
            ctx2:set_source(pattern2)
            ctx2:fill()

            return surf
        end

        -- if cutoff + ratio are tested only right and bottom cutoff can be
        -- used because the test should not rebuild the math logic of the actual
        -- function
        ---@param args table
        ---@param target_heigth integer
        ---@param target_width integer
        local function test(args, target_width, target_heigth)
            args.surface = test_square()
            local out = surface.crop_surface(args)

            -- check size
            local w, h = surface.get_size(out)
            assert.is_equal(w, target_width)
            assert.is_equal(h, target_heigth)

            -- check if area in img and cropped img are the same
            local calc_w_offset = math.ceil((50 - w - (args.right or 0))/2)
            local calc_h_offset = math.ceil((50 - h - (args.bottom or 0))/2)

            local pbuf1_in = gdk.pixbuf_get_from_surface(
                args.surface,
                args.left or calc_w_offset,
                args.top or calc_h_offset,
                w,
                h
            )
            local pbuf1_out = gdk.pixbuf_get_from_surface(out, 0, 0, w, h)
            assert.is_equal(pbuf1_in:get_pixels(), pbuf1_out:get_pixels())
        end

        it("keep size using offsets", function ()
            test({
                left = 0,
                top = 0,
            }, 50, 50)
        end)

        it("keep size using ratio", function ()
            test({
                ratio = 1,
                left = 0,
                top = 0,
            }, 50, 50)
        end)

        it("crop to 50x25 with offsets", function ()
            test({
                top = 12,
                bottom = 13,
                left = 0,
            }, 50, 25)
        end)

        it("crop to 25x50 with offsets", function ()
            test({
                left = 12,
                right = 13,
                top = 0,
            }, 25, 50)
        end)

        it("crop all edges", function ()
            test({
                left = 7,
                right = 13,
                top = 9,
                bottom = 16
            }, 30, 25)
        end)

        it("use ratio to crop width", function ()
            test({
                ratio = 3/5,
            }, 30, 50)
        end)

        it("use ratio to crop height", function ()
            test({
                ratio = 5/3,
            }, 50, 30)
        end)

        it("use very large ratio", function ()
            test({
                ratio = 10000
            }, 50, 1)
        end)

        it("use very small ratio", function ()
            test({
                ratio = 0.0001
            }, 1, 50)
        end)

        it("use ratio and offset", function ()
            test({
                bottom = 20,
                ratio = 1
            }, 30, 30)
        end)

        it("use too large offset", function ()
            assert.has.errors(function ()
                test({
                    left = 55,
                    top = 0,
                }, 0, 0)
            end)
        end)

        it("use negative offset", function ()
            assert.has.errors(function ()
                test({
                    left = -1,
                    top = 0
                }, 0, 0)
            end)
        end)

        it("use negative ratio", function ()
            assert.has.errors(function ()
                test({
                    ratio = - 1,
                }, 0, 0)
            end)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
