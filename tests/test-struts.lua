local placement   = require("awful.placement")
local wibox       = require("wibox")
local wibar       = require("awful.wibar")
local test_client = require("_client")

local steps = {}

local parent, small

local twibar, bwibar, lwibar, rwibar = screen.primary.mywibox

-- Track garbage collection.
local wibars = setmetatable({screen.primary.mywibox}, {__mode="v"})

-- Pretty print issues
local function print_expected()
    local wa, sa = mouse.screen.workarea, mouse.screen.geometry
    local ret = table.concat{
        "\nSCREEN: ", sa.x, " ", sa.y," ", sa.width, " ", sa.height
    }
    ret = table.concat{ret,
        "\nWORKAREA: ", wa.x, " ", wa.y," ", wa.width, " ", wa.height
    }
    ret = table.concat{ret,
        "\nSTRUTS: l:",
        wa.x, " t:",
        wa.y," r:",
        sa.width - wa.width - wa.x, " b:",
        sa.height - wa.height - wa.y
    }

    for k, w in ipairs {lwibar, rwibar, twibar, bwibar} do
        ret = table.concat{
            ret , "\n " , k, ": ", w.position, " [",
                w.width, ", ",
                w.height ,
            "]"
        }
    end
    return ret
end

-- All wibars on top and bottom are full width and all on left and right
-- must honor the workarea height
local function validate_wibar_geometry()
    for _, w in ipairs {lwibar, rwibar, twibar, bwibar} do
        assert(w and w.position and w.screen)
        if w.visible then
            if w.position == "top" or w.position == "bottom" then
                assert(w.width == w.screen.geometry.width, print_expected())
            else
                assert(w.height == w.screen.workarea.height, print_expected())
            end
        end
    end
end

-- Test the struts without using wibars
table.insert(steps, function()
    local sgeo = screen.primary.geometry

    -- Manually place at the bottom right
    local w = wibox{
        height  = 50,
        width   = 50,
        y       = sgeo.y + sgeo.height - 50,
        x       = sgeo.x + sgeo.width - 50,
        visible = true,
    }

    w:struts {
        left   = 0,
        top    = 0,
        bottom = 50,
        right  = 50,
    }

    local wa = screen.primary.workarea

    assert(wa.x      == 0                               )
    assert(wa.y      == twibar.height                   )
    assert(wa.width  == sgeo.width  - 50                )
    assert(wa.height == sgeo.height - 50 - twibar.height)

    w:struts {
        left   = 0,
        top    = 0,
        bottom = 0,
        right  = 0,
    }
    w.visible = false

    wa = screen.primary.workarea

    assert(wa.x      == 0                          )
    assert(wa.y      == twibar.height              )
    assert(wa.width  == sgeo.width                 )
    assert(wa.height == sgeo.height - twibar.height)

    return true
end)

-- Test "attach" for wibox/client to wibox
table.insert(steps, function()
    parent = wibox {
        visible = true,
        width   = 500,
        height  = 500,
    }

    placement.centered(parent)

    small = wibox {
        bg      = "#ff0000",
        height  = 24,
        ontop   = true,
        visible = true,
    }

    -- Add an attached function
    placement.top_left(small, {parent = parent})
    placement.maximize_horizontally(small, {parent = parent, attach = true})

    return true
end)

table.insert(steps, function()
    assert(parent:geometry().width  == 500)
    assert(parent:geometry().height == 500)

    assert(parent:geometry().y == small:geometry().y)
    assert(parent:geometry().x == small:geometry().x)
    assert(parent:geometry().width == small:geometry().width )
    assert(small:geometry().height == 24)

    -- Now, move the parent and see of the attached one is correctly updated
    placement.stretch_left(parent)

    return true
end)

table.insert(steps, function()
    assert(parent:geometry().y == small:geometry().y)
    assert(parent:geometry().x == small:geometry().x)
    assert(parent:geometry().width == small:geometry().width )
    assert(small:geometry().height == 24)

    -- Hide (and hopefully GC) the old "small"
    small.visible = false

    -- Do the same, but with placement compositing
    small = wibox {
        bg      = "#ff0000",
        height  = 50,
        width   = 50,
        ontop   = true,
        visible = true,
    }

    local p = placement.bottom_right + placement.scale

    p(small, {parent=parent, attach=true, direction="left", to_percent=0.5})

    return true
end)

local function check_ratio()
    assert(
        (
            parent:geometry().y
            + parent:geometry().height
            - small :geometry().height
            ) == small:geometry().y
    )

    assert(math.abs(parent:geometry().x + math.ceil(parent:geometry().width/2) - small:geometry().x) <= 1)
    assert(math.abs(math.ceil(parent:geometry().width/2) - small:geometry().width) <= 1)
    assert(small:geometry().height == 50)
end

table.insert(steps, function()
    check_ratio()

    -- Now, do some more transformation on the parent and make sure the "small"
    -- wibox follow.
    placement.scale    (parent, {direction="left", to_percent=0.2})
    placement.scale    (parent, {direction="up"  , to_percent=0.2})
    placement.bottom   (parent                                    )
    placement.top_right(parent                                    )
    placement.centered (parent                                    )

    return true
end)

-- Do the same checks to see if everything has followed as expected
table.insert(steps, function()
    check_ratio()

    -- Now, test if the wibar has updated the workarea.
    placement.maximize(parent, {honor_workarea = true})

    return true
end)

table.insert(steps, function()
    local wa   = screen.primary.workarea
    local sgeo = screen.primary.geometry
    local wgeo = twibar:geometry()

    assert(wa.width    == sgeo.width               )
    assert(wa.x        == sgeo.x                   )
    assert(wa.y        == sgeo.y + wgeo.height     )
    --     assert(wa.height   == sgeo.height - wgeo.height)
    --     assert(wgeo.height == sgeo.height - wa.height  )

    assert(parent.y      == wa.y                   )
    assert(parent.x      == wa.x                   )
    assert(parent.width  == wa.width               )
    assert(parent.height == wa.height              )

    -- Add more wibars
    bwibar = wibar {position = "bottom", bg = "#00ff00"}
    lwibar = wibar {position = "left"  , bg = "#0000ff"}
    rwibar = wibar {position = "right" , bg = "#ff00ff"}
    table.insert(wibars, bwibar)
    table.insert(wibars, lwibar)
    table.insert(wibars, rwibar)

    validate_wibar_geometry()

    return true
end)

-- Make sure the maximized client has the right size and position
local function check_maximize()
    local pgeo = parent:geometry()

    local margins = {left=0, right=0, top=0, bottom=0}

    for _, w in ipairs {twibar, lwibar, rwibar, bwibar} do
        if w.visible then
            local pos = w.position
            local w_or_h = (pos == "left" or pos == "right") and "width" or "height"
            margins[pos] = margins[pos] + w[w_or_h]
        end
    end

    local sgeo = parent.screen.geometry

    sgeo.x      = sgeo.x      + margins.left
    sgeo.y      = sgeo.y      + margins.top
    sgeo.width  = sgeo.width  - margins.left - margins.right
    sgeo.height = sgeo.height - margins.top  - margins.bottom

    local wa = parent.screen.workarea

    for k, v in pairs(wa) do
        assert(sgeo[k] == v)
    end

    assert(sgeo.width  == pgeo.width  + 2*parent.border_width)
    assert(sgeo.height == pgeo.height + 2*parent.border_width)
    assert(sgeo.x      == pgeo.x                             )
    assert(sgeo.y      == pgeo.y                             )
end

table.insert(steps, function()
    -- Attach the parent wibox to it is updated along the workarea
    placement.maximize(parent, {honor_workarea = true, attach = true})

    -- Make the wibox more visible
    parent.border_color = "#ffff00"
    parent.border_width = 10
    parent.ontop        = true
    --     parent.visible = false

    local wa   = screen.primary.workarea
    local sgeo = screen.primary.geometry

    assert(lwibar.width  == rwibar.width        )
    assert(lwibar.height == rwibar.height       )
    assert(twibar.x      == bwibar.x            )
    assert(twibar.width  == bwibar.width        )

    -- Check the left wibar size and position
    assert(lwibar.x      == wa.x - lwibar.width )
    assert(lwibar.height == wa.height           )

    -- Check the right wibar size and position
    assert(rwibar.x      == wa.x + wa.width     )
    assert(rwibar.height == wa.height           )

    -- Check the bottom wibar size and position
    assert(bwibar.width  == sgeo.width          )
    assert(bwibar.x      == 0                   )

    return true
end)

table.insert(steps, function()
    check_maximize()

    -- There should be a detach callback
    assert(lwibar.detach_callback)

    -- Begin to move wibars around
    lwibar.position = "top"
    assert(lwibar.position == "top")
    assert(lwibar.y == twibar.height)

    validate_wibar_geometry()

    return true
end)

table.insert(steps, function()
    check_maximize()

    bwibar.position = "right"
    bwibar.ontop = true
    rwibar.ontop = true
    assert(bwibar.position == "right")

    validate_wibar_geometry()

    return true
end)

table.insert(steps, function()
    check_maximize()

    rwibar.position = "top"

    validate_wibar_geometry()

    return true
end)

table.insert(steps, function()
    check_maximize()

    bwibar.position = "top"

    validate_wibar_geometry()

    return true
end)

table.insert(steps, function()
    check_maximize()

    for _, w in ipairs {twibar, lwibar, rwibar, bwibar} do
        w.position = "right"
        validate_wibar_geometry()
    end


    return true
end)

-- Test visibility
table.insert(steps, function()
    check_maximize()

    twibar.visible = false
    rwibar.visible = false

    return true
end)

table.insert(steps, function()
    check_maximize()

    twibar.visible = true

    return true
end)

table.insert(steps, function()
    check_maximize()

    return true
end)

-- Now test again with a real client

local c
table.insert(steps, function()
    -- I'm lazy, so get rid of all the wiboxes that have struts
    parent.visible = false
    small.visible = false
    bwibar.visible = false
    lwibar.visible = false
    rwibar.visible = false
    screen.primary.mywibox.visible = false

    -- Spawn a client to test things with
    assert(#client.get() == 0)
    test_client()

    return true
end)

-- Given a geometry and a workarea, test that the given struts are applied
local function test_workarea(geo, wa, left, right, top, bottom)
    -- Get the line number from where we were called (helps debugging)
    local line = debug.getinfo(2).currentline

    assert(geo.x + left == wa.x,
        string.format("%d + %d == %d called from line %d", geo.x, left, wa.x, line))
    assert(geo.y + top  == wa.y,
        string.format("%d + %d == %d called from line %d", geo.y, top, wa.y, line))
    assert(geo.width - left - right == wa.width,
        string.format("%d - %d - %d == %d called from line %d", geo.width, left, right, wa.width, line))
    assert(geo.height - top - bottom == wa.height,
        string.format("%d - %d - %d == %d called from line %d", geo.height, top, bottom, wa.height, line))
end

table.insert(steps, function()
    if #client.get() ~= 1 then
        return
    end

    c = client.get()[1]
    assert(c)
    assert(c:isvisible())

    -- Test some simple struts
    c:struts { left = 50 }
    test_workarea(c.screen.geometry, c.screen.workarea, 50, 0, 0, 0)
    validate_wibar_geometry()

    -- A tag switch should make the client no longer apply
    screen.primary.tags[2]:view_only()
    test_workarea(c.screen.geometry, c.screen.workarea, 0, 0, 0, 0)
    validate_wibar_geometry()

    -- But sticky clients always 'count'
    c.sticky = true
    test_workarea(c.screen.geometry, c.screen.workarea, 50, 0, 0, 0)
    validate_wibar_geometry()

    c.sticky = false
    test_workarea(c.screen.geometry, c.screen.workarea, 0, 0, 0, 0)
    validate_wibar_geometry()

    -- And switch back to the right tag
    c.first_tag:view_only()
    test_workarea(c.screen.geometry, c.screen.workarea, 50, 0, 0, 0)
    validate_wibar_geometry()

    -- What if we move the client to another tag?
    c:tags{ screen.primary.tags[2] }
    test_workarea(c.screen.geometry, c.screen.workarea, 0, 0, 0, 0)
    validate_wibar_geometry()

    -- Move it back to the selected tag
    c:tags{ screen.primary.tags[1] }
    test_workarea(c.screen.geometry, c.screen.workarea, 50, 0, 0, 0)
    validate_wibar_geometry()

    return true
end)

local s

table.insert(steps, function()
    c = client.get()[1]
    assert(c)
    s = c.screen
    c:kill()

    bwibar:remove()
    lwibar:remove()
    rwibar:remove()
    twibar:remove()

    for _, w in ipairs(wibars) do
        assert(not w.visible)
    end

    -- Make sure the placement doesn't hold a reference.
    bwibar, lwibar, rwibar, twibar = nil, nil, nil, nil
    screen.primary.mywibox = nil

    return true
end)

table.insert(steps, function()
    for _=1, 3 do
        collectgarbage("collect")
    end

    if next(wibars) then
        for _, w in ipairs(wibars) do
            assert(not w.visible)
        end

        return
    end

    return true
end)

table.insert(steps, function()
    if client.get()[1] then
        return
    end

    test_workarea(s.geometry, s.workarea, 0, 0, 0, 0)
    validate_wibar_geometry()

    local wdg = {
        layout = wibox.layout.align.vertical,
        wibox.widget.textbox("BEGIN"),
        nil,
        wibox.widget.textbox("END")
    }

    local wdg2 = {
        layout = wibox.layout.align.horizontal,
        wibox.widget.textbox("BEGIN"),
        nil,
        wibox.widget.textbox("END")
    }

    lwibar = wibar{position = "top", screen = s, height = 15,
        visible = true, bg = "#660066"}
    lwibar:setup(wdg2)
    table.insert(wibars, lwibar)

    rwibar = wibar{position = "top", screen = s, height = 15,
        visible = true, bg = "#660000"}
    rwibar:setup(wdg2)
    table.insert(wibars, rwibar)

    bwibar = wibar{position = "left", screen = s, ontop = true, width = 64,
        visible = true, bg = "#006600"}
    bwibar:setup(wdg)
    table.insert(wibars, bwibar)

    twibar = wibar{position = "bottom", screen = s,
        height = 15, bg = "#666600"}
    twibar:setup(wdg2)
    table.insert(wibars, twibar)

    test_workarea(s.geometry, s.workarea, 64, 0, 30, 15)
    validate_wibar_geometry()

    bwibar:remove()

    test_workarea(s.geometry, s.workarea, 0, 0, 30, 15)
    validate_wibar_geometry()

    lwibar:remove()

    test_workarea(s.geometry, s.workarea, 0, 0, 15, 15)
    validate_wibar_geometry()

    rwibar:remove()

    test_workarea(s.geometry, s.workarea, 0, 0, 0, 15)
    validate_wibar_geometry()

    twibar:remove()

    test_workarea(s.geometry, s.workarea, 0, 0, 0, 0)
    validate_wibar_geometry()

    return true
end)

-- Test resizing wibars.
table.insert(steps, function()
    -- Make sure the placement doesn't hold a reference.
    bwibar, lwibar, rwibar, twibar = nil, nil, nil, nil

    for _=1, 3 do
        collectgarbage("collect")
    end

    assert(not next(wibars))

    twibar = wibar{position = "top", screen = s, height = 15,
        visible = true, bg = "#660066"}

    assert(twibar.height == 15)

    twibar.height = 64
    assert(twibar.height == 64)

    twibar:geometry { height = 128 }
    assert(twibar.height == 128)

    local old_width = twibar.width
    twibar.width = 42
    assert(twibar.width == old_width)


    return true
end)

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
