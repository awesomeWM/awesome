-- Test that client shapes work

local runner = require("_runner")
local spawn = require("awful.spawn")
local surface = require("gears.surface")

runner.run_steps{
    function(count)
        if count == 1 then
            local a = spawn("xeyes")
            assert(type(a) == "number", a)
        end
        if #client.get() == 1 then
            local c = client.get()[1]

            local geo = c:geometry()

            -- Resize it to test shape change events.
            c:geometry {
                x      = geo.x,
                y      = geo.y,
                width  = geo.width * 2,
                height = geo.height * 2,
            }

            return true
        end
    end,

    function()
        -- Here would be the right place to actually test anything.
        -- However, this test just exists to stabilise code coverage (without
        -- it, it would always fluctuate). Thus, this is not really a test...
        local c = client.get()[1]
        assert(c.name == "xeyes", c.name)
        assert(surface.load_silently(c.client_shape_bounding, false))
        assert(surface.load_silently(c.shape_bounding, false))

        assert(not surface.load_silently(c.client_shape_clip, false))
        assert(not surface.load_silently(c.shape_clip, false))

        return true
    end,

    function()
        client.get()[1]:kill()
        return true
    end


}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
