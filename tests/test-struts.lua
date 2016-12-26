local placement   = require("awful.placement")
local wibox       = require("wibox")
local wibar       = require("awful.wibar")

local steps = {}

local parent, small

local twibar, bwibar, lwibar, rwibar = screen.primary.mywibox

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

return true
end)

table.insert(steps, function()
check_maximize()

bwibar.position = "right"
bwibar.ontop = true
rwibar.ontop = true
assert(bwibar.position == "right")

return true
end)

table.insert(steps, function()
check_maximize()

rwibar.position = "top"

return true
end)

table.insert(steps, function()
check_maximize()

bwibar.position = "top"

return true
end)

table.insert(steps, function()
check_maximize()

for _, w in ipairs {twibar, lwibar, rwibar, bwibar} do
    w.position = "right"
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


require("_runner").run_steps(steps)
