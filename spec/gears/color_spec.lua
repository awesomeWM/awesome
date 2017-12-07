---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local color = require("gears.color")
local cairo = require("lgi").cairo
local say = require("say")

describe("gears.color", function()
    describe("parse_color", function()
        local function test(e_r, e_g, e_b, e_a, input)
            local o_r, o_g, o_b, o_a, unused = color.parse_color(input)
            assert.is.same(o_r, e_r / 0xff)
            assert.is.same(o_g, e_g / 0xff)
            assert.is.same(o_b, e_b / 0xff)
            assert.is.same(o_a, e_a / 0xff)
            assert.is_nil(unused)
        end

        it("black", function()
            test(0, 0, 0, 0xff, "#000000")
        end)

        it("black", function()
            test(0, 0, 0, 0xff, "black")
        end)

        it("opaque black", function()
            test(0, 0, 0, 0xff, "#000000ff")
        end)

        it("transparent gray", function()
            test(0x80, 0x80, 0x80, 0x80, "#80808080")
        end)

        it("transparent white", function()
            test(0xff, 0xff, 0xff, 0x7f, "#ffffff7f")
        end)

        it("chartreuse", function()
            test(0x7f, 0xff, 0x00, 0xff, "chartreuse")
        end)

        describe("invalid input", function()
            local function test_nil(input)
                local output = color.parse_color(input)
                assert.is_nil(output)
            end

            it("nonexisting color", function()
                test_nil("elephant")
            end)

            it("invalid format", function()
                test_nil('#f0f0f')
            end)

            it("too long", function()
                test_nil("#00000fffff00000fffff")
            end)
        end)


        describe("different lengths", function()
            local function gray(e, e_a, input)
                local o_r, o_g, o_b, o_a, unused = color.parse_color(input)
                assert.is.same(o_r, e)
                assert.is.same(o_g, e)
                assert.is.same(o_b, e)
                assert.is.same(o_a, e_a)
                assert.is_nil(unused)
            end

            it("rgb", function()
                gray(0x8/0xf, 1, "#888")
            end)

            it("rgba", function()
                gray(0x8/0xf, 0x8/0xf, "#8888")
            end)

            it("rrggbb", function()
                gray(0x80/0xff, 1, "#808080")
            end)

            it("rrggbbaa", function()
                gray(0x80/0xff, 0x80/0xff, "#80808080")
            end)

            it("rrrrggggbbbb", function()
                gray(0x8000/0xffff, 1, "#800080008000")
            end)

            it("rrrrggggbbbbaaaa", function()
                gray(0x8000/0xffff, 0x8000/0xffff, "#8000800080008000")
            end)
        end)
    end)

    local function test_pattern_stops(pattern, stops)
        for i, stop in pairs(stops) do
            local status, offset, r, g, b, a = pattern:get_color_stop_rgba(i - 1)
            assert.is.same("SUCCESS", status)
            assert.is.same(offset, stop.offset)
            assert.is.same({ r, g, b, a }, stop.rgba)
        end

        assert.is.same({ pattern:get_color_stop_count() },
                { "SUCCESS", #stops })
    end

    local function test_linear_pattern(pattern, from, to, stops)
        assert.is.equal(pattern:get_type(), "LINEAR")
        assert.is.same({ pattern:get_linear_points() },
                { "SUCCESS", from[1], from[2], to[1], to[2] })
        test_pattern_stops(pattern, stops)
    end

    describe("linear pattern", function()
        it("table description", function()
            local pattern = color({
                type = "linear",
                from = { 2, 10 }, to = { 102, 110 },
                stops = {
                    { 0,   "#ff0000" },
                    { 0.5, "#00ff00" },
                    { 1,   "#0000ff" },
                }
            })
            test_linear_pattern(pattern, { 2, 10 }, { 102, 110}, {
                    { offset = 0,   rgba = { 1, 0, 0, 1 }},
                    { offset = 0.5, rgba = { 0, 1, 0, 1 }},
                    { offset = 1,   rgba = { 0, 0, 1, 1 }}
                })
        end)

        it("string description", function()
            local pattern = color("linear:2,10:102,110:0,#ff0000:0.5,#00ff00:1,#0000ff")
            test_linear_pattern(pattern, { 2, 10 }, { 102, 110 }, {
                    { offset = 0,   rgba = { 1, 0, 0, 1 }},
                    { offset = 0.5, rgba = { 0, 1, 0, 1 }},
                    { offset = 1,   rgba = { 0, 0, 1, 1 }}
                })
        end)
    end)

    local function test_radial_pattern(pattern, from, to, stops)
        assert.is.equal(pattern:get_type(), "RADIAL")
        assert.is.same({ pattern:get_radial_circles() },
                { "SUCCESS", from[1], from[2], from[3], to[1], to[2], to[3] })
        test_pattern_stops(pattern, stops)
    end

    describe("radial pattern", function()
        it("table description", function()
            local pattern = color({
                type = "radial",
                from = { 2, 10, 42 }, to = { 102, 110, 142 },
                stops = {
                    { 0,   "#ff0000" },
                    { 0.5, "#00ff00" },
                    { 1,   "#0000ff" },
                }
            })
            test_radial_pattern(pattern, { 2, 10, 42 }, { 102, 110, 142 }, {
                    { offset = 0,   rgba = { 1, 0, 0, 1 }},
                    { offset = 0.5, rgba = { 0, 1, 0, 1 }},
                    { offset = 1,   rgba = { 0, 0, 1, 1 }}
                })
        end)

        it("string description", function()
            local pattern = color("radial:2,10,42:102,110,142:0,#ff0000:0.5,#00ff00:1,#0000ff")
            test_radial_pattern(pattern, { 2, 10, 42 }, { 102, 110, 142 }, {
                    { offset = 0,   rgba = { 1, 0, 0, 1 }},
                    { offset = 0.5, rgba = { 0, 1, 0, 1 }},
                    { offset = 1,   rgba = { 0, 0, 1, 1 }}
                })
        end)
    end)

    describe("create_opaque_pattern", function()
        -- Assertion to check if a pattern is opaque
        local function opaque(_, arguments)
            assert(arguments.n >= 1, say("assertions.argtolittle", { "opaque", 1, tostring(arguments.n) }))
            local pattern = color.create_opaque_pattern(arguments[1])
            return pattern ~= nil
        end
        say:set("assertion.opaque.positive", "Pattern %s should be opaque, but isn't.")
        say:set("assertion.opaque.negative", "Pattern %s should NOT be opaque, but is.")
        assert:register("assertion", "opaque", opaque, "assertion.opaque.positive", "assertion.opaque.negative")

        it("opaque solid pattern", function()
            assert.is.opaque("#ffffff")
        end)

        it("transparent solid pattern", function()
            assert.is_not.opaque("#ffffff7f")
        end)

        it("opaque linear pattern", function()
            assert.is.opaque("#ffffff")
        end)

        describe("transparent linear pattern", function()
            it("without stops", function()
                assert.is_not.opaque("linear:0,0:0,10:")
            end)

            it("with transparent stops", function()
                assert.is_not.opaque("linear:0,0:0,10:0,#00ff00ff:1,#ff00ff00")
            end)

            it("with NONE repeat", function()
                local pattern = color.create_pattern_uncached("linear:0,0:0,10:0,#00ff00ff:1,#ff00ffff")
                pattern:set_extend("NONE")
                assert.is_not.opaque(pattern)
            end)
        end)

        it("opaque linear pattern", function()
            assert.is.opaque("linear:0,0:0,10:0,#00ff00ff:1,#ff00ffff")
        end)

        it("opaque surface pattern", function()
            local surface = cairo.ImageSurface(cairo.Format.RGB24, "1", "1")
            local pattern = cairo.Pattern.create_for_surface(surface)
            pattern:set_extend("PAD")
            assert.is.opaque(pattern)
        end)

        describe("transparent surface pattern", function()
            it("with alpha channel", function()
                local surface = cairo.ImageSurface(cairo.Format.ARGB32, "1", "1")
                local pattern = cairo.Pattern.create_for_surface(surface)
                pattern:set_extend("PAD")
                assert.is_not.opaque(pattern)
            end)

            it("with NONE repeat", function()
                local surface = cairo.ImageSurface(cairo.Format.RGB24, "1", "1")
                local pattern = cairo.Pattern.create_for_surface(surface)
                assert.is_not.opaque(pattern)
            end)
        end)

        -- cairo_pattern_create_mesh is new in cairo 1.12 / lgi 0.7.0
        local create_mesh = cairo.Pattern.create_mesh
        if create_mesh then
            it("unsupported pattern type", function()
                assert.is_not.opaque(create_mesh())
            end)
        else
            pending("unsupported pattern type")
        end
    end)

    describe("pattern cache", function()
        it("caching works", function()
            assert.is.equal(color("#00ff00"), color("#00ff00"))
        end)

        it("create_pattern_uncached does not cache", function()
            -- Since tests run in order, the above test already inserted
            -- "#00ff00" into the cache
            assert.is_not.equal(color.create_pattern_uncached("#00ff00"), color.create_pattern_uncached("#00ff00"))
        end)
    end)

    describe("ensure_pango_color", function()
        -- Successful cases
        for _, value in ipairs{ "red", "cyan", "black", "#f00", "#014578",
                    "#01ef01ef01ef", "#f00f", "#014578ab", "#01ef01ef01ef01ef"
                } do
            it(value, function()
                assert.is.same(value, color.ensure_pango_color(value))
            end)
        end

        it("#abz", function()
            assert.is.same("black", color.ensure_pango_color("#abz"))
        end)

        it("#fedcba98765432", function()
            -- Only one, two or four characters per channel are supported
            assert.is.same("black", color.ensure_pango_color("#fedcba98765432"))
        end)

        it("fallback", function()
            assert.is.same("zzz", color.ensure_pango_color("#abz", "zzz"))
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
