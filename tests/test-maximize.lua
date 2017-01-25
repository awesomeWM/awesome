--- Tests maximize and fullscreen

local runner = require("_runner")
local awful = require("awful")

local original_geo = nil

local steps = {
    function(count)
        if count == 1 then
            awful.spawn("xterm")
        else
            local c = client.get()[1]
            if c then
                original_geo = c:geometry()
                return true
            end
        end
    end,

    -- maximize horizontally
    function()
        local c = client.get()[1]
        assert(not c.maximized_horizontal)
        assert(not c.maximized_vertical  )
        assert(not c.maximized           )
        assert(not c.fullscreen          )

        c.maximized_horizontal = true
        return true
    end,
    function()
        local c = client.get()[1]

        --local new_geo = c:geometry()
        --local sgeo = c.screen.workarea

        --assert(new_geo.x-c.border_width==sgeo.x) --FIXME c:geometry({x=1}).x ~= 1

        --assert(new_geo.width+2*c.border_width==sgeo.width) --FIXME off by 4px

        c.maximized_horizontal = false
        return true
    end,

    -- Test restoring client.border_width
    function()
        local c = client.get()[1]

        -- pick an arbitrary border_width distinct from the default one
        local test_width = c.border_width + 1

        c.border_width = test_width

        c.fullscreen = true
        c.fullscreen = false

        assert(c.border_width == test_width)

        return true
    end,

    -- Test restoring a geometry
    function()
        local c = client.get()[1]

        local new_geo = c:geometry()

        for k,v in pairs(original_geo) do
            assert(new_geo[k] == v)
        end

        c.fullscreen = true

        return true
    end,
    function()
        local c = client.get()[1]

        local new_geo = c:geometry()
        local sgeo    = c.screen.geometry
        local bw      = c.border_width

        assert(c.fullscreen)

        assert(new_geo.x==sgeo.x)
        assert(new_geo.y==sgeo.y)
        assert(new_geo.x+new_geo.width+2*bw==sgeo.x+sgeo.width)
        assert(new_geo.y+new_geo.height+2*bw==sgeo.y+sgeo.height)

        c.fullscreen = false

        return true
    end,
    function()
        local c = client.get()[1]

        local new_geo = c:geometry()

        for k,v in pairs(original_geo) do
            assert(new_geo[k] == v)
        end

        c.floating  = true

        awful.placement.centered(c)
        original_geo = c:geometry()

        c.maximized = true


        return true
    end,
    function()
        local c = client.get()[1]

        local new_geo = c:geometry()
        local sgeo    = c.screen.workarea

        assert(c.maximized)
        assert(c.floating)
        assert(new_geo.x==sgeo.x)
        assert(new_geo.y==sgeo.y)

        c.maximized = false

        return true
    end,
    function()
        local c = client.get()[1]

        assert(not c.maximized)
        assert(c.floating)

        local new_geo = c:geometry()

        for k,v in pairs(original_geo) do
            assert(new_geo[k] == v)
        end

        return true
    end
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
