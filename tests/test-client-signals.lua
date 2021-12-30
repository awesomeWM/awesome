local runner = require("_runner")
local test_client = require("_client")
local c = nil
local recorded_signals = {}

runner.run_steps {
    -- Spawns the a client
    function(count)
        if count == 1 then
            test_client()
        end
        c = client.get()[1]
        return c
    end,
    -- Sets up initial geometry and callbacks
    function()
        c:geometry {x = 100, y = 100, width = 100, height = 100}
        for _, property in ipairs {"x", "y", "width", "height"} do
            local signal_name = "property::" .. property
            c:connect_signal(
                signal_name,
                function ()
                    recorded_signals[signal_name] = true
                end)
        end
        return true
    end,
    -- Change x only
    function()
        recorded_signals = {}
        c:geometry {x = 0}
        return true
    end,
    function()
        assert(recorded_signals["property::x"])
        assert(not recorded_signals["property::y"])
        assert(not recorded_signals["property::width"])
        assert(not recorded_signals["property::height"])
        return true
    end,
    -- Change y only
    function()
        recorded_signals = {}
        c:geometry {y = 0}
        return true
    end,
    function()
        assert(not recorded_signals["property::x"])
        assert(recorded_signals["property::y"])
        assert(not recorded_signals["property::width"])
        assert(not recorded_signals["property::height"])
        return true
    end,
    -- Change width only
    function()
        recorded_signals = {}
        c:geometry {width = 200}
        return true
    end,
    function()
        assert(not recorded_signals["property::x"])
        assert(not recorded_signals["property::y"])
        assert(recorded_signals["property::width"])
        assert(not recorded_signals["property::height"])
        return true
    end,
    -- Change height only
    function()
        recorded_signals = {}
        c:geometry {height = 200}
        return true
    end,
    function()
        assert(not recorded_signals["property::x"])
        assert(not recorded_signals["property::y"])
        assert(not recorded_signals["property::width"])
        assert(recorded_signals["property::height"])
        return true
    end,
    -- Check for regression when all x/y/width/height are changed.
    function()
        recorded_signals = {}
        c:geometry {x = 100, y = 100, width = 100, height = 100}
        return true
    end,
    function()
        assert(recorded_signals["property::x"])
        assert(recorded_signals["property::y"])
        assert(recorded_signals["property::width"])
        assert(recorded_signals["property::height"])
        return true
    end,
    -- Clean up
    function()
        c:kill()
        return true
    end
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
