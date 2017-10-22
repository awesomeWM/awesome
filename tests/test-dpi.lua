--- Tests DPI functions

local runner = require("_runner")
local beautiful = require("beautiful")
local dpi = require("gears").dpi

local function must_fail(func)
    assert(not pcall(func))
end

local function invalid_policy(val)
    must_fail(function() dpi.rounding_policy = val end)
end

-- user policy for DPI rounding: always return 42
local function theanswer(_)
    return 42
end

local s = screen.primary

local steps = {
    -- Basic font DPI tests
    function()
        local font_dpi = beautiful.get_font_dpi()
        local s_font_dpi = beautiful.get_font_dpi(s)
        -- the primary screen font DPI should always match the global font DPI
        assert(font_dpi == s_font_dpi)

        return true
    end,

    -- gears.dpi testing
    function()
        -- core DPI will be ~75 when using Xephyr, who-knows-what when using
        -- Xvfb
        local core = dpi.core_dpi()

        -- cannot get/set arbitrary keys in gears.dpi
        must_fail(function() dpi.whatever = 'blah' end)
        must_fail(function() print(dpi.whatever) end)
        -- check the default rounding policy
        assert(dpi.rounding_policy == 'identity')

        -- should not be able to set the rounding policy to user
        -- unless a user function has been defined
        invalid_policy('user')
        -- cannot set a random policy
        invalid_policy('whatever')
        -- policy must be a string, nil or function
        invalid_policy(42)
        invalid_policy({})

        dpi.rounding_policy = theanswer

        assert(dpi.core_dpi() == 42)
        assert(s.dpi == 42)

        dpi.rounding_policy = nil

        assert(dpi.core_dpi() == core)

        -- can now set the rounding policy to user
        dpi.rounding_policy = 'user'
        assert(dpi.core_dpi() == 42)

        -- users will most frequenly want the rounding policy 'round',
        -- if any at all
        dpi.rounding_policy = 'round'
        -- DPIs should now be integer multiples of 96
        local rounded = dpi.core_dpi()
        assert(math.floor(rounded/96) == math.ceil(rounded/96))
        rounded = s.dpi
        assert(math.floor(rounded/96) == math.ceil(rounded/96))

        if core < 96 then
            dpi.rounding_policy = 'floor'
            -- nobody said this was a good idea
            assert(dpi.core_dpi() == 0)
            dpi.rounding_policy = 'ceil'
            assert(dpi.core_dpi() == 96)
        elseif core < 192 then
            dpi.rounding_policy = 'floor'
            assert(dpi.core_dpi() == 96)
            dpi.rounding_policy = 'ceil'
            assert(dpi.core_dpi() == 192)
        else -- >= 192
            dpi.rounding_policy = 'floor'
            assert(dpi.core_dpi() >= 192)
            dpi.rounding_policy = 'ceil'
            assert(dpi.core_dpi() >= 192)
        end

        return true
    end
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
