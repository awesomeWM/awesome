---------------------------------------------------------------------------
-- @author Alex Belykh
-- @copyright 2021 Alex Belykh
---------------------------------------------------------------------------

local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local bezier = require("gears.math.bezier")
local random = math.random

-- Values in some test cases were rounded for shortness,
-- so this tolerance is used when comparing them.
local test_precision = 0.0001
-- This tolerance is used for comparing results
-- of different computations of the mathematically same thing.
local high_precision = 0.0000001

local test_cases_curve_evaluate_at = {
    -- Cubics at random points.
    {t=0.25,c={-1,-1,-4,-6},r=-1.5},
    {t=0.2,c={-1,0,6,-8},r=0},
    {t=0.75,c={7,3,-3,-3},r=-2},
    {t=0.85,c={-3,1,5,9},r=7.2},
    {t=0.6,c={5,7,1,2},r=3.2},
    {t=0.4,c={-3,9,-7,9},r=1.8},
    {t=0.75,c={4,7,-1,8},r=4},
    {t=0.8,c={10,6,9,-1},r=3.6},
    {t=0.2,c={-10,7,10,9},r=-1.4},
    {t=0.6,c={0,-4,-10,-8},r=-7.2},

    -- Cubics at the halfpoint.
    {t=0.5,c={9,-6,-3,6},r=-1.5},
    {t=0.5,c={3,5,-8,2},r=-0.5},
    {t=0.5,c={2,-4,3,1},r=0},
    {t=0.5,c={0,10,-4,6},r=3},
    {t=0.5,c={-9,9,1,-9},r=1.5},
    {t=0.5,c={-6,3,8,-3},r=3},
    {t=0.5,c={-5,10,8,7},r=7},
    {t=0.5,c={-5,5,3,-3},r=2},
    {t=0.5,c={4,-7,-1,8},r=-1.5},
    {t=0.5,c={-2,7,4,9},r=5},

    -- Various degree curves at random points.
    {t=0.6,c={1,0},r=0.4},
    {t=0.5,c={7,2,-4,3},r=0.5},
    {t=0.8,c={8,-5,-3},r=-3.2},
    {t=0.45,c={-10},r=-10},
    {t=0.95,c={-5},r=-5},
    {t=0.9,c={-1},r=-1},
    {t=0.2,c={1,-2},r=0.4},
    {t=0.2,c={6},r=6},
    {t=0.5,c={3,10,3,5,-9,-8},r=2.5},
    {t=0.2,c={-9},r=-9},

    -- Various degree curves at halfpoint.
    {t=0.5,c={6},r=6},
    {t=0.5,c={-2,-8},r=-5},
    {t=0.5,c={10},r=10},
    {t=0.5,c={0,7},r=3.5},
    {t=0.5,c={-4,3,1,0,10},r=1.5},
    {t=0.5,c={6,1,6},r=3.5},
    {t=0.5,c={-5},r=-5},
    {t=0.5,c={1,-10,-1,9,9},r=0},
    {t=0.5,c={-1,-6,-7},r=-5},
    {t=0.5,c={1,-2},r=-0.5},
}

local test_cases_curve_split_at = {
    -- Cubics at random points.
    {t=0.2,c={-1,0,6,-8},r={-1,-0.8,-0.4,0}},
    {t=0.85,c={-3,1,5,9},r={-3,0.4,3.8,7.2}},
    {t=0.9,c={-5,-2,1,4},r={-5,-2.3,0.4,3.1}},
    {t=0.6,c={-1,7,-10,-2},r={-1,3.8,-0.4,-2.8}},
    {t=0.65,c={-3,-1,1,3},r={-3,-1.7,-0.4,0.9}},
    {t=0.3,c={10,7,4,1},r={10,9.1,8.2,7.3}},
    {t=0.2,c={5,-4,2,-2},r={5,3.2,2,1.2}},
    {t=0.2,c={-1,1,3,5},r={-1,-0.6,-0.2,0.2}},
    {t=0.8,c={1,10,9,-2},r={1,8.2,9,3.4}},
    {t=0.6,c={-5,10,-5,0},r={-5,4,2.2,0.4}},

    -- Cubics at the halfpoint.
    {t=0.5,c={9,-6,-3,6},r={9,1.5,-1.5,-1.5}},
    {t=0.5,c={0,10,-4,6},r={0,5,4,3}},
    {t=0.5,c={-9,9,1,-9},r={-9,0,2.5,1.5}},
    {t=0.5,c={-6,3,8,-3},r={-6,-1.5,2,3}},
    {t=0.5,c={-5,5,3,-3},r={-5,0,2,2}},
    {t=0.5,c={-2,7,4,9},r={-2,2.5,4,5}},
    {t=0.5,c={10,-6,8,-8},r={10,2,1.5,1}},
    {t=0.5,c={-10,9,2,-7},r={-10,-0.5,2.5,2}},
    {t=0.5,c={-4,10,2,8},r={-4,3,4.5,5}},
    {t=0.5,c={4,0,0,-4},r={4,2,1,0}},

    -- Various degree curves at random points.
    {t=0.6,c={1,0},r={1,0.4}},
    {t=0.4,c={6,1,6},r={6,4,3.6}},
    {t=0.2,c={4,2,0,-2,-4},r={4,3.6,3.2,2.8,2.4}},
    {t=0.05,c={7,5,3,1,0,-1},r={7,6.9,6.8,6.7,6.6,6.5}},
    {t=0.4,c={-6,-7,-8,-9,-10},r={-6,-6.4,-6.8,-7.2,-7.6}},
    {t=0.45,c={-10},r={-10}},
    {t=0.05,c={-8,-6,-4,-2,-7},r={-8,-7.9,-7.8,-7.7,-7.6}},
    {t=0.2,c={1,-2},r={1,0.4}},
    {t=0.75,c={10,-6,10,-6,10},r={10,-2,4,1,2.5}},
    {t=0.45,c={-6},r={-6}},

    -- Various degree curves at the halfpoint.
    {t=0.5,c={-2,-8},r={-2,-5}},
    {t=0.5,c={-5},r={-5}},
    {t=0.5,c={-1,-6,-7},r={-1,-3.5,-5}},
    {t=0.5,c={3,-2,5,8,7},r={3,0.5,1,2.5,4}},
    {t=0.5,c={-2,10,-8,8,10},r={-2,4,2.5,1.5,2}},
    {t=0.5,c={-9,9,-7,-1,3},r={-9,0,0.5,-0.5,-1}},
    {t=0.5,c={-8,-1,8,-9,8,-1},r={-8,-4.5,-0.5,0.5,0.5,0.5}},
    {t=0.5,c={0,2,0,-2,-8,2},r={0,1,1,0.5,-0.5,-1.5}},
    {t=0.5,c={3,-3,3,5,3,-3},r={3,0,0,1,2,2.5}},
    {t=0.5,c={4,1,6,-5,-8,5,-6},r={4,2.5,3,2.5,1,-0.5,-1.5}},
}

local test_cases_curve_derivative = {
    {{9}},
    {{-10}},
    {{-2,-5},{-3}},
    {{-2,-8},{-6}},
    {{-1,-3.5,-5},{-5,-3},{2}},
    {{-1,-6,-7},{-10,-2},{8}},
    {{-10,-0.5,2.5,2},{28.5,9,-1.5},{-39,-21},{18}},
    {{-10,9,2,-7},{57,-21,-27},{-156,-12},{144}},
    {{-9,0,0.5,-0.5,-1},{36,2,-4,-2},{-102,-18,6},{168,48},{-120}},
    {{-9,9,-7,-1,3},{72,-64,24,16},{-408,264,-24},{1344,-576},{-1920}},
    {{-8,-1,8,-9,8,-1},{35,45,-85,85,-45},{40,-520,680,-520},{-1680,3600,-3600},{10560,-14400},{-24960}},
    {{-8,-4.5,-0.5,0.5,0.5,0.5},{17.5,20,5,0,0},{10,-60,-20,0},{-210,120,60},{660,-120},{-780}},
    {
        {4,2.5,3,2.5,1,-0.5,-1.5},{-9,3,-3,-9,-9,-6},{60,-30,-30,0,15},
        {-360,0,120,60},{1080,360,-180},{-1440,-1080},{360}
    },
    {
        {4,1,6,-5,-8,5,-6},{-18,30,-66,-18,78,-66},{240,-480,240,480,-720},
        {-2880,2880,960,-4800},{17280,-5760,-17280},{-46080,-23040},{23040}
    },
}

local test_cases_curve_derivative_at = {
    {c={9},tr={{0.8,9},{0.4,0}}},
    {c={-10},tr={{0.8,-10},{0.8,0}}},
    {c={-2,-5},tr={{0.9,-4.7},{0.2,-3},{0.3,0}}},
    {c={-2,-8},tr={{0.8,-6.8},{0.3,-6},{0.6,0}}},
    {c={-1,-6,-7},tr={{0,-1},{0.2,-8.4},{0.1,8},{0.8,0}}},
    {c={-1,-3.5,-5},tr={{1,-5},{0.9,-3.2},{0.6,2},{0.7,0}}},
    {c={-10,9,2,-7},tr={{0.5,2},{0,57},{0.5,-84},{0.1,144},{0.2,0}}},
    {c={-10,-0.5,2.5,2},tr={{1,2},{1,-1.5},{0.3,-33.6},{0.8,18},{0.5,0}}},
    {c={-9,9,-7,-1,3},tr={{0.5,-1},{0,72},{0.4,-24},{0.9,-384},{0.9,-1920},{0.7,0}}},
    {c={-9,0,0.5,-0.5,-1},tr={{1,-1},{0.5,3.5},{0.4,-44.4},{0.8,72},{0.5,-120},{0.7,0}}},
    {c={-8,-4.5,-0.5,0.5,0.5,0.5},tr={{1,0.5},{1,0},{1,0},{0.6,45.6},{0.8,36},{0.1,-780},{0.1,0}}},
    {c={-8,-1,8,-9,8,-1},tr={{1,-1},{0.5,0},{1,-520},{0.9,-2284.8},{0.2,5568},{0.9,-24960},{0.6,0}}},
    {c={4,1,6,-5,-8,5,-6},tr={{1,-6},{0,-18},{1,-720},{1,-4800},{0.7,-9331.2},{0.5,-34560},{0.2,23040},{0.8,0}}},
    {c={4,2.5,3,2.5,1,-0.5,-1.5},tr={{1,-1.5},{1,-6},{0,60},{0.5,7.5},{0.7,160.2},{0.1,-1404},{0.5,360},{0.8,0}}},
}

local test_cases_curve_elevate_degree = {
    {c={0},r={0,0}},
    {c={-1},r={-1,-1}},
    {c={9},r={9,9}},

    {c={3,-7},r={3,-2,-7}},
    {c={-5,3},r={-5,-1,3}},
    {c={0,-2},r={0,-1,-2}},
    {c={2,-7},r={2,-2.5,-7}},

    {c={9,-10},r={9,-0.5,-10}},
    {c={7,-5,10},r={7,-1,0,10}},
    {c={10,-8,1},r={10,-2,-5,1}},

    {c={3,3,-7,4,0},r={3,3,-3,-2.6,3.2,0}},
    {c={4,-8,-8,-10},r={4,-5,-8,-8.5,-10}},
    {c={4,-4,8,3,-3},r={4,-2.4,3.2,6,1.8,-3}},
    {c={-9,8,9,-9,9},r={-9,4.6,8.6,1.8,-5.4,9}},

    {c={7,7,-2,7,-5,-5},r={7,7,1,2.5,3,-5,-5}},
    {c={-2,7,-2,7,4,-8},r={-2,5.5,1,2.5,6,2,-8}},
    {c={8,-1,2,-9,9,6},r={8,0.5,1,-3.5,-3,8.5,6}},

    {c={9,9,2,9,9,2,9},r={9,9,4,6,9,7,3,9}},
    {c={2,9,9,-5,2,2,2},r={2,8,9,1,-2,2,2,2}},
    {c={1,-6,1,8,1,8,8},r={1,-5,-1,5,5,3,8,8}},
    {c={2,9,9,-5,2,-5,9},r={2,8,9,1,-2,0,-3,9}},
}

local test_cases_cubic_through_points = {
    {p={1},r={1,1,1,1}},
    {p={3},r={3,3,3,3}},
    {p={4},r={4,4,4,4}},
    {p={-9},r={-9,-9,-9,-9}},

    {p={0,6},r={0,2,4,6}},
    {p={6,6},r={6,6,6,6}},
    {p={5,-1},r={5,3,1,-1}},
    {p={3,-3},r={3,1,-1,-3}},
    {p={-6,3},r={-6,-3,0,3}},
    {p={0,-6},r={0,-2,-4,-6}},

    {p={4,1,7},r={4,-1,0,7}},
    {p={3,0,3},r={3,-1,-1,3}},
    {p={3,9,-6},r={3,14,11,-6}},
    {p={-10,2,5},r={-10,1,6,5}},
    {p={-7,-1,5},r={-7,-3,1,5}},
    {p={-6,6,-3},r={-6,9,10,-3}},
    {p={-9,-6,0},r={-9,-8,-5,0}},
    {p={-3,-6,0},r={-3,-8,-7,0}},
    {p={-10,-7},r={-10,-9,-8,-7}},
    {p={-8,-5,-5},r={-8,-5,-4,-5}},
    {p={-7,-4,-1},r={-7,-5,-3,-1}},

    {p={-3,1,-1,3},r={-3,8,-8,3}},
    {p={2,10,-8,8},r={2,43,-45,8}},
    {p={-1,-2,-1,2},r={-1,-3,-2,2}},
    {p={-8,6,5,-2},r={-8,16.5,5,-2}},
    {p={-7,-7,-3,-1},r={-7,-11,0,-1}},
    {p={-4,-6,-5,2},r={-4,-6.5,-9,2}},
    {p={8,-8,-4,-7},r={8,-27,8.5,-7}},
    {p={-1,8,4,-1},r={-1,18.5,0.5,-1}},
    {p={-3,0,-2,-3},r={-3,4.5,-4.5,-3}},
    {p={6,6,-5,-9},r={6,17.5,-14.5,-9}},
}

local test_cases_cubic_from_derivative_and_points_min_stretch = {
    {p={9,1,2},r={1,4,1,2}},
    {p={6,4,4},r={4,6,3.5,4}},
    {p={0,8,2},r={8,8,3.5,2}},
    {p={0,-4,4},r={-4,-4,2,4}},
    {p={-9,8,9},r={8,5,9.5,9}},
    {p={-3,-6,3},r={-6,-7,1,3}},
    {p={0,-1,-9},r={-1,-1,-7,-9}},
    {p={-6,-5,3},r={-5,-7,1.5,3}},
    {p={0,-2,-4},r={-2,-2,-3.5,-4}},
    {p={6,-10,2},r={-10,-8,-1.5,2}},
}

local test_cases_cubic_from_derivative_and_points_min_strain = {
    {p={6,4,4},r={4,6,5,4}},
    {p={-9,8,9},r={8,5,7,9}},
    {p={0,6,3},r={6,6,4.5,3}},
    {p={3,4,-4},r={4,5,0.5,-4}},
    {p={9,-8,8},r={-8,-5,1.5,8}},
    {p={-3,-6,3},r={-6,-7,-2,3}},
    {p={9,-8,2},r={-8,-5,-1.5,2}},
    {p={9,-1,-5},r={-1,2,-1.5,-5}},
    {p={3,-3,-9},r={-3,-2,-5.5,-9}},
    {p={-3,10,-10},r={10,9,-0.5,-10}},
}

local test_cases_cubic_from_derivative_and_points_min_jerk = {
    {p={0,6,3},r={6,6,5,3}},
    {p={6,4,4},r={4,6,6,4}},
    {p={0,8,2},r={8,8,6,2}},
    {p={-9,6,6},r={6,3,3,6}},
    {p={9,-6,6},r={-6,-3,1,6}},
    {p={-3,-6,3},r={-6,-7,-4,3}},
    {p={3,-3,-9},r={-3,-2,-4,-9}},
    {p={6,-10,2},r={-10,-8,-4,2}},
    {p={9,-8,-5},r={-8,-5,-4,-5}},
    {p={-3,-10,-10},r={-10,-11,-11,-10}},
}


local function assert_array_is_near(expected, got, precision)
    assert.is.equal(#expected, #got)
    for i, e in ipairs(expected) do
        assert.is.near(e, got[i], precision)
    end
end

local function table_reverse(t)
    local r = {}
    local l = #t
    for i = l,1,-1 do
        r[i] = t[l-i+1]
    end
    return r
end

describe("gears.math.bezier", function()
    describe("curve_evaluate_at", function()
        it("computes correctly", function()
            for _, tst in ipairs(test_cases_curve_evaluate_at) do
                assert.is.near(tst.r, bezier.curve_evaluate_at(tst.c, tst.t), test_precision)
            end
        end)

        it("equals to the 1st control point when t=0", function()
            for _, tst in ipairs(test_cases_curve_evaluate_at) do
                assert.is.equal(tst.c[1], bezier.curve_evaluate_at(tst.c, 0))
            end
        end)

        it("equals to the last control point when t=1", function()
            for _, tst in ipairs(test_cases_curve_evaluate_at) do
                assert.is.equal(tst.c[#tst.c], bezier.curve_evaluate_at(tst.c, 1))
            end
        end)
    end) -- end describe(curve_evaluate_at)

    describe("curve_split_at", function()
        it("computes the left curve correctly", function()
            for _, tst in ipairs(test_cases_curve_split_at) do
                local subcurve = bezier.curve_split_at(tst.c, tst.t)
                assert_array_is_near(tst.r, subcurve, test_precision)

                -- We know it's a valid subcurve, because it evaluates
                -- to the same as the original curve at corresponding points.
                for i = 0, 64 do
                    local sub_t = i/64
                    local orig_t = sub_t * tst.t

                    local expected = bezier.curve_evaluate_at(tst.c, orig_t)
                    local got = bezier.curve_evaluate_at(subcurve, sub_t)
                    assert.is.near(expected, got, test_precision)
                end
            end
        end)

        it("computes the right curve like the left", function()
            for _, tst_left in ipairs(test_cases_curve_split_at) do
                -- Mirror t and curve points to get a case
                local tst = {
                    t=1-tst_left.t,
                    c=table_reverse(tst_left.c),
                    r=table_reverse(tst_left.r)
                }
                local _, subcurve = bezier.curve_split_at(tst.c, tst.t)
                assert.is.equal(#tst.r, #subcurve)
                for i, expected in ipairs(tst.r) do
                    assert.is.near(expected, subcurve[i], test_precision)
                end
            end
        end)
    end) -- end describe(curve_split_at)

    describe("curve_derivative", function()
        it("computes correctly", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                local c = tst[1]
                for n = 0, #tst-1 do
                    -- These tests contain exact values, so use high_precision.
                    assert_array_is_near(tst[n+1], bezier.curve_derivative(c, n), high_precision)
                end
            end
        end)

        it("returns {} when c={}", function()
            for _, i in ipairs({1, 2, 10, 55, 120}) do
                assert.is.same({}, bezier.curve_derivative({}, i))
            end
        end)

        it("returns {} when n >= degree+1", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    local degree = #c-1
                    for _, i in ipairs({1, 2, 10, 55, 120}) do
                        assert.is.same({}, bezier.curve_derivative(c, degree+i))
                    end
                end
            end
        end)

        it("returns nil when n<0", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    for _, n in ipairs({-1, -2, -10, -55, -120}) do
                        assert.is_nil(bezier.curve_derivative(c, n))
                    end
                end
            end
        end)

        it("defaults to n=1", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    -- There should be literally no difference in performed
                    -- operations, so compare floats exactly.
                    assert.is.same(bezier.curve_derivative(c,1), bezier.curve_derivative(c))
                end
            end
        end)

        it("returns the same result for n as deriving twice with n1+n2 == n", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                local c = tst[1]
                for n = 0, #tst-1 do
                    local expected = bezier.curve_derivative(c, n)
                    for n1 = 0, n do
                        local n2 = n-n1
                        local intermediate_curve = bezier.curve_derivative(c, n1)
                        local got = bezier.curve_derivative(intermediate_curve, n2)
                        -- There should be literally no difference in performed
                        -- operations, so compare floats exactly.
                        assert.is.same(expected, got)
                    end
                end
            end
        end)
    end) -- end describe(curve_derivative)

    describe("curve_derivative_at", function()
        it("computes correctly", function()
            for _, tst in ipairs(test_cases_curve_derivative_at) do
                for n, tr in ipairs(tst.tr) do
                    local t, expected = unpack(tr)
                    assert.is.near(expected, bezier.curve_derivative_at(tst.c, t, n-1), test_precision)
                end
            end
        end)

        it("returns nil when c={}", function()
            for _, n in ipairs({0, 1, 3, 14, 31, 121}) do
                local t = random()
                assert.is_nil(bezier.curve_derivative_at({}, t, n))
            end
        end)

        it("returns zero argument when set and c={}", function()
            local zero = {}
            for _, n in ipairs({0, 1, 3, 14, 31, 121}) do
                local t = random()
                assert.is_equal(zero, bezier.curve_derivative_at({}, t, n, zero))
            end
            -- And never else.
            for _, tst in ipairs(test_cases_curve_derivative_at) do
                for n, tr in ipairs(tst.tr) do
                    local t, _ = unpack(tr)
                    assert.is_not.equal(zero, bezier.curve_derivative_at(tst.c, t, n-1, zero))
                end
            end
        end)

        it("returns 0 when n >= degree+1", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    local degree = #c-1
                    for _, i in ipairs({1, 2, 10, 55, 120}) do
                        local t = random()
                        assert.is.equal(0, bezier.curve_derivative_at(c, t, degree+i))
                    end
                end
            end
        end)

        it("returns nil when n<0", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    for _, n in ipairs({-1, -2, -10, -55, -120}) do
                        local t = random()
                        assert.is_nil(bezier.curve_derivative_at(c, t, n))
                    end
                end
            end
        end)

        it("defaults to n=1", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    local t = random()
                    assert.is.equal(bezier.curve_derivative_at(c, t, 1), bezier.curve_derivative_at(c, t))
                end
            end
        end)

        it("is almost the same as curve_evaluate_at(curve_derivative(c, n), t) or 0", function()
            -- but for 0<#c<=n+1 curve_derivative_at() will return 0, whereas
            -- curve_evaluate(curve_derivative()) will return nil.

            for _, tst in ipairs(test_cases_curve_derivative_at) do
                for n, tr in ipairs(tst.tr) do
                    local t, _ = unpack(tr)
                    local expected = bezier.curve_evaluate_at(bezier.curve_derivative(tst.c, n-1), t) or 0
                    assert.is.near(expected, bezier.curve_derivative_at(tst.c, t, n-1), high_precision)
                end
            end
        end)
    end) -- end describe(curve_derivative_at)

    describe("curve_derivative_at_zero", function()
        it("is the same as curve_derivative_at(c, 0)", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    local expected = bezier.curve_derivative_at_zero(c)
                    local got = bezier.curve_derivative_at(c, 0)
                    assert.is.near(expected, got, high_precision)
                end
            end
        end)

        it("returns zero argument when set and c={}", function()
            local zero = {}
            assert.is_equal(zero, bezier.curve_derivative_at_zero({}, zero))
            -- And never else.
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    assert.is_not.equal(zero, bezier.curve_derivative_at_zero(c, zero))
                end
            end
        end)
    end) -- end describe(curve_derivative_at_zero)

    describe("curve_derivative_at_one", function()
        it("is the same as curve_derivative_at(c, 1)", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    local expected = bezier.curve_derivative_at_one(c)
                    local got = bezier.curve_derivative_at(c, 1)
                    assert.is.near(expected, got, high_precision)
                end
            end
        end)

        it("returns zero argument when set and c={}", function()
            local zero = {}
            assert.is_equal(zero, bezier.curve_derivative_at_one({}, zero))
            -- And never else.
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    assert.is_not.equal(zero, bezier.curve_derivative_at_one(c, zero))
                end
            end
        end)
    end) -- end describe(curve_derivative_at_one)

    describe("curve_elevate_degree", function()
        it("computes correctly", function()
            for _, tst in ipairs(test_cases_curve_elevate_degree) do
                -- These tests contain exact values, so use high_precision.
                assert_array_is_near(tst.r, bezier.curve_elevate_degree(tst.c), high_precision)
            end
        end)

        it("returns a curve that is equal everywhere to the original curve", function()
            for _, tst in ipairs(test_cases_curve_derivative) do
                for _, c in ipairs(tst) do
                    for i = 0, 64 do
                        local t = i/64
                        local elevated_c = bezier.curve_elevate_degree(c)
                        local expected = bezier.curve_evaluate_at(c, t)
                        local got = bezier.curve_evaluate_at(elevated_c, t)
                        assert.is.near(expected, got, high_precision)
                    end
                end
            end
        end)
    end) -- end describe(curve_elevate_degree)

    describe("cubic_through_points", function()
        it("computes correctly", function()
            for _, tst in ipairs(test_cases_cubic_through_points) do
                local got = {bezier.cubic_through_points(unpack(tst.p))}
                -- These tests contain exact values, so use high_precision.
                assert_array_is_near(tst.r, got, high_precision)

                -- First and last control points are actually equal to endpoints,
                -- and #got is 4, and we know that from tests anyway, but jic.
                assert.is.equal(4, #got)
                assert.is.equal(tst.p[1], got[1])
                assert.is.equal(tst.p[#tst.p], got[4])

                if #tst.p == 4 then
                    -- Further arguments are ignored.
                    local weird_args = {tst.p[1], tst.p[2], tst.p[3], tst.p[4], 5, 6, 7, 8}
                    assert_array_is_near(tst.r, {bezier.cubic_through_points(unpack(weird_args))}, high_precision)
                end
            end
        end)

        it("returns nil, when given no points", function()
            assert.is_nil(bezier.cubic_through_points())
        end)
    end) -- end describe(cubic_through_points)

    describe("cubic_from_points_and_derivatives", function()
        it("is proven valid with curve_derivative_at_*()", function()
            -- the test_cases_curve_split_at table has a lot of cubic curves.
            for _, tst in ipairs(test_cases_curve_split_at) do
                for _, expected in ipairs({tst.c, tst.r}) do
                    if #expected == 4 then
                        local p0, p3 = expected[1], expected[4]
                        local d0 = bezier.curve_derivative_at_zero(expected)
                        local d3 = bezier.curve_derivative_at_one(expected)
                        local got = {bezier.cubic_from_points_and_derivatives(d0, p0, p3, d3)}
                        assert_array_is_near(expected, got, high_precision)
                    end
                end
            end
        end)
    end) -- end describe(cubic_from_points_and_derivatives)

    describe("cubic_from_derivative_and_points_min_stretch", function()
        it("computes correctly", function()
            for _, tst in ipairs(test_cases_cubic_from_derivative_and_points_min_stretch) do
                local got = {bezier.cubic_from_derivative_and_points_min_stretch(unpack(tst.p))}
                -- These tests contain exact values, so use high_precision.
                assert_array_is_near(tst.r, got, high_precision)

                -- First and last control points should be equal to endpoints,
                -- the derivative at t=0 should be as requested,
                -- and #got is 4, and we know that from tests anyway, but jic.
                assert.is.equal(4, #got)
                assert.is.equal(tst.p[1], bezier.curve_derivative_at_zero(got))
                assert.is.equal(tst.p[2], got[1])
                assert.is.equal(tst.p[3], got[4])

                -- Further arguments are ignored.
                local weird_args = {tst.p[1], tst.p[2], tst.p[3], 5, 6, 7, 8}
                assert_array_is_near(
                    tst.r,
                    {bezier.cubic_from_derivative_and_points_min_stretch(unpack(weird_args))},
                    high_precision
                )
            end
        end)

        -- This test is a bit heavy and is known to pass, so I'm commenting it out.
        --[[
        it("produces smallest stretch energy curve compared to other generators", function()
            local function calc_stretch(c)
                local d = bezier.curve_derivative(c, 1)
                -- Integrate numerically.
                local pieces = 1000
                local r = 0
                for i = 1, pieces do
                    r = r + bezier.curve_evaluate_at(d, i/pieces)^2
                end
                return r/pieces
            end
            for d0 = -10, 10 do
                for p0 = -10, 10 do
                    for p3 = -10, 10 do
                        local c_ch = {bezier.cubic_from_derivative_and_points_min_stretch(d0, p0, p3)}
                        local v_ch = calc_stretch(c_ch)
                        local c_n = {bezier.cubic_from_derivative_and_points_min_strain(d0, p0, p3)}
                        local v_n = calc_stretch(c_n)
                        local c_j = {bezier.cubic_from_derivative_and_points_min_jerk(d0, p0, p3)}
                        local v_j = calc_stretch(c_j)
                        -- Break ties in degenerate cases, when all generators
                        -- produce the same curve, but poluted with floating point noise.
                        v_ch = v_ch - high_precision
                        assert(v_ch <= v_n and v_ch <= v_j)
                    end
                end
            end
        end)
        ]]--
    end) -- end describe(cubic_from_derivative_and_points_min_stretch)

    describe("cubic_from_derivative_and_points_min_strain", function()
        it("computes correctly", function()
            for _, tst in ipairs(test_cases_cubic_from_derivative_and_points_min_strain) do
                local got = {bezier.cubic_from_derivative_and_points_min_strain(unpack(tst.p))}
                -- These tests contain exact values, so use high_precision.
                assert_array_is_near(tst.r, got, high_precision)

                -- First and last control points should be equal to endpoints,
                -- the derivative at t=0 should be as requested,
                -- and #got is 4, and we know that from tests anyway, but jic.
                assert.is.equal(4, #got)
                assert.is.equal(tst.p[1], bezier.curve_derivative_at_zero(got))
                assert.is.equal(tst.p[2], got[1])
                assert.is.equal(tst.p[3], got[4])

                -- Further arguments are ignored.
                local weird_args = {tst.p[1], tst.p[2], tst.p[3], 5, 6, 7, 8}
                assert_array_is_near(
                    tst.r,
                    {bezier.cubic_from_derivative_and_points_min_strain(unpack(weird_args))},
                    high_precision
                )
            end
        end)

        -- This test is a bit heavy and is known to pass, so I'm commenting it out.
        --[[
        it("produces smallest strain energy curve compared to other generators", function()
            local function calc_strain(c)
                local d = bezier.curve_derivative(c, 2)
                -- Integrate numerically.
                local pieces = 1000
                local r = 0
                for i = 1, pieces do
                    r = r + bezier.curve_evaluate_at(d, i/pieces)^2
                end
                return r/pieces
            end
            for d0 = -10, 10 do
                for p0 = -10, 10 do
                    for p3 = -10, 10 do
                        local c_ch = {bezier.cubic_from_derivative_and_points_min_stretch(d0, p0, p3)}
                        local v_ch = calc_strain(c_ch)
                        local c_n = {bezier.cubic_from_derivative_and_points_min_strain(d0, p0, p3)}
                        local v_n = calc_strain(c_n)
                        local c_j = {bezier.cubic_from_derivative_and_points_min_jerk(d0, p0, p3)}
                        local v_j = calc_strain(c_j)
                        -- Break ties in degenerate cases, when all generators
                        -- produce the same curve, but poluted with floating point noise.
                        v_n = v_n - high_precision
                        assert(v_n <= v_ch and v_n <= v_j)
                    end
                end
            end
        end)
        ]]--
    end) -- end describe(cubic_from_derivative_and_points_min_strain)

    describe("cubic_from_derivative_and_points_min_jerk", function()
        it("computes correctly", function()
            for _, tst in ipairs(test_cases_cubic_from_derivative_and_points_min_jerk) do
                local got = {bezier.cubic_from_derivative_and_points_min_jerk(unpack(tst.p))}
                -- These tests contain exact values, so use high_precision.
                assert_array_is_near(tst.r, got, high_precision)

                -- First and last control points should be equal to endpoints,
                -- the derivative at t=0 should be as requested,
                -- and #got is 4, and we know that from tests anyway, but jic.
                assert.is.equal(4, #got)
                assert.is.equal(tst.p[1], bezier.curve_derivative_at_zero(got))
                assert.is.equal(tst.p[2], got[1])
                assert.is.equal(tst.p[3], got[4])

                -- Further arguments are ignored.
                local weird_args = {tst.p[1], tst.p[2], tst.p[3], 5, 6, 7, 8}
                assert_array_is_near(
                    tst.r,
                    {bezier.cubic_from_derivative_and_points_min_jerk(unpack(weird_args))},
                    high_precision
                )
            end
        end)

        -- This test is a bit heavy and is known to pass, so I'm commenting it out.
        --[[
        it("produces smallest jerk energy curve compared to other generators", function()
            local function calc_jerk(c)
                -- the 3-rd derivative of a cubic curve is constant,
                -- so there's no need for numeric integration.
                local r = bezier.curve_derivative(c, 3)[1]
                return r^2
            end
            for d0 = -10, 10 do
                for p0 = -10, 10 do
                    for p3 = -10, 10 do
                        local c_ch = {bezier.cubic_from_derivative_and_points_min_stretch(d0, p0, p3)}
                        local v_ch = calc_jerk(c_ch)
                        local c_n = {bezier.cubic_from_derivative_and_points_min_strain(d0, p0, p3)}
                        local v_n = calc_jerk(c_n)
                        local c_j = {bezier.cubic_from_derivative_and_points_min_jerk(d0, p0, p3)}
                        local v_j = calc_jerk(c_j)
                        -- Break ties in degenerate cases, when all generators
                        -- produce the same curve, but poluted with floating point noise.
                        v_j = v_j - high_precision
                        assert(v_j <= v_ch and v_j <= v_n)
                    end
                end
            end
        end)
        ]]--
    end) -- end describe(cubic_from_derivative_and_points_min_jerk)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
