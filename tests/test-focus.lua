--- Tests for focus signals / property.
-- Test for https://github.com/awesomeWM/awesome/issues/134.

local runner = require("_runner")
local awful = require("awful")
local gdebug = require("gears.debug")

local beautiful = require("beautiful")
beautiful.border_color_normal = "#0000ff"
beautiful.border_color_active = "#00ff00"

client.connect_signal("focus", function(c)
    c.border_color = "#ff0000"
end)

local function assert_equals(a, b)
    if a == b then
        return
    end
    error(string.format("Assertion failed: %s == %s", a, b))
end


local steps = {
    -- border_color should get applied via focus signal for first client on tag.
    function(count)
        if count == 1 then
            awful.spawn("xterm")
        else
            local c = client.get()[1]
            if c then
                assert_equals(c.border_color, "#ff0000")
                return true
            end
        end
    end,

    -- border_color should get applied via focus signal for second client on tag.
    function(count)
        if count == 1 then
            awful.spawn("xterm")
        else
            if #client.get() == 2 then
                local c = client.get()[1]
                assert_equals(c, client.focus)
                if c then
                    assert_equals(c.border_color, "#ff0000")
                    return true
                end

            end
        end
    end,

    -- Test if alpha works as intended
    function()
        local c = client.get()[1]

        local function test(set, expected)
            expected = expected or set
            c.border_color = set
            assert_equals(c.border_color, expected)
        end

        test("#123456")
        test("#12345678")
        test("#123456ff", "#123456")

        return true
    end,

    function()
        assert(client.focus)
        local called, called2 = false, false
        gdebug.print_warning = function() called = true end
        local c2 = client.focus

        client.focus.active = false
        assert(called)
        assert(not client.focus)
        assert(not c2.active)
        called = false

        local real_assert = assert
        assert = function() called2 = true end --luacheck: globals assert

        c2.active = true

        assert = real_assert --luacheck: globals assert

        assert(called2)
        assert(not called)
        assert(c2.active)
        assert(client.focus == c2)

        return true
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
