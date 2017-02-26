--- Tests maximize and fullscreen

local runner = require("_runner")
local awful = require("awful")
local test_client = require("_client")
local lgi = require("lgi")

local original_geo = nil

local function check_original_geo()
    local c = client.get()[1]
    local new_geo = c:geometry()

    for k,v in pairs(original_geo) do
        assert(new_geo[k] == v, table.concat{
            "gravity :\n", (c.size_hints.win_gravity or "N/A"), ",\n",
            "border_width :\n", c.border_width, ",\n",
            original_geo.x     , " == ", new_geo.x     , ",\n",
            original_geo.y     , " == ", new_geo.y     , ",\n",
            original_geo.width , " == ", new_geo.width , ",\n",
            original_geo.height, " == ", new_geo.height, ",\n",
        })
    end
end

local steps = {
    function(count)
        if count == 1 then
            test_client(nil,nil,nil,nil,nil,{gravity=lgi.Gdk.Gravity.NORTH_WEST})
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
        check_original_geo()

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

    -- Test restoring client.border_width (again)
    function()
        local c = client.get()[1]
        check_original_geo()

        -- same as last test, but add 1 again. This tests odd and even numbers
        -- for off-by-one and see if the geometry cache is still tinted by the
        -- old values
        local test_width = c.border_width + 1

        c.border_width = test_width

        c.fullscreen = true
        c.fullscreen = false

        assert(c.border_width == test_width)
        check_original_geo()

        return true
    end,

    -- Test restoring a geometry
    function()
        local c = client.get()[1]
        check_original_geo()

        c.fullscreen = true

        return true
    end,
    function()
        local c = client.get()[1]

        local new_geo = c:geometry()
        local sgeo    = c.screen.geometry
        local bw      = c.border_width

        assert(c.fullscreen)

        local assert_msg = {
            "gravity :\n", (c.size_hints.win_gravity or "N/A"), ",\n",
            "border_width :\n", c.border_width, ",\n",
            sgeo.x     , " == ", new_geo.x     , ",\n",
            sgeo.y     , " == ", new_geo.y     , ",\n",
            sgeo.x+sgeo.width+2*bw , " == ", new_geo.width , ",\n",
            sgeo.y+sgeo.height+2*bw, " == ", new_geo.height, ",\n",
        }

        assert(new_geo.x==sgeo.x, table.concat(assert_msg))
        assert(new_geo.y==sgeo.y, table.concat(assert_msg))
        assert(new_geo.x+new_geo.width+2*bw==sgeo.x+sgeo.width, table.concat(assert_msg))
        assert(new_geo.y+new_geo.height+2*bw==sgeo.y+sgeo.height, table.concat(assert_msg))

        c.fullscreen = false

        return true
    end,
    function()
        local c = client.get()[1]
        check_original_geo()

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
        check_original_geo()

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
