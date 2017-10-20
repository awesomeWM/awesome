--- Tests maximize and fullscreen

local runner = require("_runner")
local awful = require("awful")
local test_client = require("_client")
local lgi = require("lgi")
local gears = require("gears")

local function geo_to_str(g)
    return "pos=" .. g.x .. "," .. g.y ..
        ";size=" .. g.width .. "x" .. g.height
end

local original_geo = nil

local steps = {}

for _, gravity in ipairs { "NORTH_WEST", "NORTH", "NORTH_EAST", "WEST",
    "CENTER", "EAST", "SOUTH_WEST", "SOUTH", "SOUTH_EAST", "STATIC" } do
    gears.table.merge(steps, {
    function(count)
        if count == 1 then
            print("testing gravity " .. gravity)
            test_client(nil,nil,nil,nil,nil,{gravity=lgi.Gdk.Gravity[gravity]})
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
        assert(not c.immobilized_horizontal)
        assert(not c.immobilized_vertical)

        c.maximized_horizontal = true
        assert(c.immobilized_horizontal)
        assert(not c.immobilized_vertical)
        return true
    end,
    function()
        local c = client.get()[1]

        --local new_geo = c:geometry()
        --local sgeo = c.screen.workarea

        --assert(new_geo.x-c.border_width==sgeo.x) --FIXME c:geometry({x=1}).x ~= 1

        --assert(new_geo.width+2*c.border_width==sgeo.width) --FIXME off by 4px

        c.maximized_horizontal = false
        assert(not c.immobilized_horizontal)
        return true
    end,

    -- Test restoring client.border_width
    function()
        local c = client.get()[1]

        -- pick an arbitrary border_width distinct from the default one
        local test_width = c.border_width + 1

        c.border_width = test_width

        c.fullscreen = true

        -- Test that the client covers the full screen
        assert(geo_to_str(c:geometry()) == geo_to_str(c.screen.geometry),
            geo_to_str(c:geometry()) .. " == " .. geo_to_str(c.screen.geometry))

        c.fullscreen = false

        assert(c.border_width == test_width)

        -- Restore old border width
        c.border_width = test_width - 1

        return true
    end,

    -- Test restoring a geometry
    function()
        local c = client.get()[1]

        local new_geo = c:geometry()

        assert(geo_to_str(original_geo) == geo_to_str(new_geo),
            geo_to_str(original_geo) .. " == " .. geo_to_str(new_geo))

        c.fullscreen = true

        return true
    end,
    function()
        local c = client.get()[1]

        local new_geo = c:geometry()
        local sgeo    = c.screen.geometry
        local bw      = c.border_width

        assert(c.fullscreen)
        assert(c.immobilized_horizontal)
        assert(c.immobilized_vertical)

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

        assert(geo_to_str(original_geo) == geo_to_str(new_geo),
            geo_to_str(original_geo) .. " == " .. geo_to_str(new_geo))

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
        assert(c.immobilized_horizontal)
        assert(c.immobilized_vertical)

        c.maximized = false

        return true
    end,
    function()
        local c = client.get()[1]

        assert(not c.maximized)
        assert(c.floating)

        local new_geo = c:geometry()

        assert(geo_to_str(original_geo) == geo_to_str(new_geo),
            geo_to_str(original_geo) .. " == " .. geo_to_str(new_geo))

        c:kill()

        return true
    end
})
end

local counter = 0

local function geometry_handler(c, context, hints)
    hints = hints or {}
    assert(type(c) == "client")
    assert(type(context) == "string")
    assert(type(hints.toggle) == "boolean")
    assert(type(hints.status) == "boolean")
    counter = counter + 1
end

gears.table.merge(steps, {
    -- Now, start some clients maximized
    function()
        if #client.get() > 0 then return end

        test_client(nil,nil,nil,nil,nil,{maximize_before=true})

        return true
    end,
    function()
        local c = client.get()[1]

        if not c then return end

        assert(not c.maximized_horizontal)
        assert(not c.maximized_vertical)
        assert(c.maximized)
        assert(c.immobilized_horizontal)
        assert(c.immobilized_vertical)

        c:kill()

        return true
    end,
    function()
        if #client.get() > 0 then return end

        test_client(nil,nil,nil,nil,nil,{maximize_after=true})

        return true
    end,
    function()
        local c = client.get()[1]

        if not c then return end

        assert(not c.maximized_horizontal)
        assert(not c.maximized_vertical)

        -- It might happen in the second try
        if not c.maximized then return end

        assert(c.immobilized_horizontal)
        assert(c.immobilized_vertical)

        c:kill()

        return true
    end,
    function()
        if #client.get() > 0 then return end

        -- Test if resizing requests work
        test_client(nil,nil,nil,nil,nil,{resize={width=400, height=400}})

        return true
    end,
    function()
        if #client.get() ~= 1 then return end

        local c = client.get()[1]
        local _, size = c:titlebar_top()

        if c.width ~= 400 or c.height ~= 400+size then return end

        c:kill()

        return true
    end,
    function()
        if #client.get() > 0 then return end

        -- Remove the default handler and replace it with a testing one.
        -- **WARNING**: add tests **BEFORE** this function if you want them
        -- to be relevant.
        client.disconnect_signal("request::geometry", awful.ewmh.geometry)
        client.disconnect_signal("request::geometry", awful.ewmh.merge_maximization)
        client.connect_signal("request::geometry", geometry_handler)

        test_client(nil,nil,nil,nil,nil,{maximize_after=true})

        return true
    end,
    function()
        local c = client.get()[1]

        if not c or counter ~= 2 then return end

        return true
    end
})

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
