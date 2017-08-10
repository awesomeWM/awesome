local gears = require("gears")

local delayed_call
local cb_count = 0

local steps = {
    -- Basic functionality of gears.timer.delayed_call.
    function(_)
        gears.timer.delayed_call(function()
            delayed_call = true
        end)
        return true
    end,
    function(_)
        assert(delayed_call)
        return true
    end,

    -- gears.timer.delayed_call: re-queue function.
    function(_)
        local function cb()
            cb_count = cb_count + 1
            if cb_count < 3 then
                gears.timer.delayed_call(function()
                    cb()
                end)
            end
        end

        gears.timer.delayed_call(function()
            cb()
        end)
        return true
    end,
    function(_)
        assert(cb_count == 2, 'Count should be 2, but is: ' .. cb_count)
        return true
    end,
    function(_)
        assert(cb_count == 3, cb_count)
        return true
    end,
    function(_)
        assert(cb_count == 3, cb_count)
        return true
    end,
}

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
