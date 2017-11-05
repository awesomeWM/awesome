local test_client = require("_client")
local placement = require("awful.placement")
local amouse = require("awful.mouse")
local rounded_rect = require("gears.shape").rounded_rect

local steps = {}

table.insert(steps, function(count)
    if count == 1 then  -- Setup.
        test_client("foobar", "foobar")
    elseif #client.get() > 0 then
        client.get()[1] : geometry {
            x      = 200,
            y      = 200,
            width  = 300,
            height = 300,
        }

        client.get()[1].shape = rounded_rect

        return true
    end
end)

table.insert(steps, function()
-- The mousegrabber expect a button to be pressed.
root.fake_input("button_press",1)
local c = client.get()[1]

-- Just in case there is an accidental delayed geometry callback
assert(c:geometry().x      == 200)
assert(c:geometry().y      == 200)
assert(c:geometry().width  == 300)
assert(c:geometry().height == 300)

mouse.coords {x = 500+2*c.border_width, y= 500+2*c.border_width}

local corner = amouse.client.resize(c)

assert(corner == "bottom_right")

return true
end)

-- The geometry should remain the same, as the cursor is placed at the end of
-- the geometry.
table.insert(steps, function()

    local c = client.get()[1]

    assert(c:geometry().x      == 200)
    assert(c:geometry().y      == 200)
    assert(c:geometry().width  == 300)
    assert(c:geometry().height == 300)

    mouse.coords {x = 600+2*c.border_width, y= 600+2*c.border_width}

    return true
end)

-- Grow the client by 100px
table.insert(steps, function()

    local c = client.get()[1]

    assert(c:geometry().x      == 200)
    assert(c:geometry().y      == 200)
    assert(c:geometry().width  == 400)
    assert(c:geometry().height == 400)

    mouse.coords {x = 400+2*c.border_width, y= 400+2*c.border_width}

    return true
end)

-- Shirnk the client by 200px
table.insert(steps, function()

    local c = client.get()[1]

    assert(c:geometry().x      == 200)
    assert(c:geometry().y      == 200)
    assert(c:geometry().width  == 200)
    assert(c:geometry().height == 200)

    mouse.coords {x = 100, y= 100}

    return true
end)

-- Grow the client by 100px from the top left
table.insert(steps, function()

    local c = client.get()[1]

    assert(c:geometry().x      == 100)
    assert(c:geometry().y      == 100)
    assert(c:geometry().width  == 300)
    assert(c:geometry().height == 300)

    mouse.coords {x = 300, y= 200}

    return true
end)

-- Shrink the client by 100px from the top right
table.insert(steps, function()

    local c = client.get()[1]

    assert(c:geometry().x == 100)
    assert(c:geometry().y == 200)
    --     assert(c:geometry().width == 200-2*c.border_width) --FIXME off by border width...
    --     assert(c:geometry().height == 200-2*c.border_width) --FIXME off by border width...


    -- Now do a couple check with the "after" mode to make sure it doesn't
    -- regress.
    root.fake_input("button_release",1)

    mousegrabber.stop()

    amouse.resize.set_mode("after")
    c.border_width = 0
    c:geometry {
        x      = 100,
        y      = 200,
        width  = 300,
        height = 200,
    }
    assert(c:geometry().x      == 100)
    assert(c:geometry().y      == 200)
    assert(c:geometry().width  == 300)
    assert(c:geometry().height == 200)

    root.fake_input("button_press",1)
    amouse.client.resize(c)

    mouse.coords {x = 500, y= 500}

    return true
end)

-- Grow the client by 100px from the top left ("after" mode)
table.insert(steps, function()

    -- local c = client.get()[1]

    --     if not mousegrabber.isrunning then --FIXME it should work, but doesn't
    --         return true
    --     end

    --FIXME, the mousegrabber callback says the mouse buttons are not pressed,
    -- theirfor the test is broken
    -- Nothing should have changed until button_release is done
    --assert(c:geometry().x      == 100)
    --assert(c:geometry().y      == 200)
    --assert(c:geometry().width  == 300)
    --assert(c:geometry().height == 200)

    mouse.coords {x = 300, y= 200}

    return true
end)

-- Stop the resize
table.insert(steps, function()

    local c = client.get()[1]

    root.fake_input("button_release",1)

    assert(c:geometry().x      == 100)
    assert(c:geometry().y      == 200)
    assert(c:geometry().width  == 400)
    assert(c:geometry().height == 300)

    amouse.resize.set_mode("live")

    mousegrabber.stop()

    return true
end)

-- Setup for move testing
table.insert(steps, function()
    assert(not mousegrabber.isrunning())

    local c = client.get()[1]

    c:geometry {
        width  = 200,
        height = 200,
    }

    placement.bottom_right(c)

    mouse.coords {x = c.screen.geometry.width -150,
    y = c.screen.geometry.height-150}


    return true
end)

-- Start the move mouse grabber
table.insert(steps, function()
    local c = client.get()[1]

    -- The resize is over, it should not move the client anymore
    assert(c:geometry().x == c.screen.geometry.width  - 200 - 2*c.border_width)
    assert(c:geometry().y == c.screen.geometry.height - 200 - 2*c.border_width)
    assert(c:geometry().width  == 200)
    assert(c:geometry().height == 200)

    assert(c.valid)

    root.fake_input("button_press",1)

    -- Begin the move
    amouse.client.move(c)

    -- Make sure nothing unwanted happen by accident
    assert(c:geometry().x == c.screen.geometry.width  - 200 - 2*c.border_width)
    assert(c:geometry().y == c.screen.geometry.height - 200 - 2*c.border_width)
    assert(c:geometry().width  == 200)
    assert(c:geometry().height == 200)

    -- The cursor should not have moved
    assert(mouse.coords().x == c.screen.geometry.width  - 150)
    assert(mouse.coords().y == c.screen.geometry.height - 150)
    mouse.coords {x = 50 + 2*c.border_width, y= 50 + 2*c.border_width}

    assert(mousegrabber.isrunning())

    return true
end)

-- The client should now be in the top left
table.insert(steps, function()
    local c = client.get()[1]
    assert(c:geometry().x == 0)
    assert(c:geometry().y == 0)
    assert(c:geometry().width  == 200)
    assert(c:geometry().height == 200)

    -- Move to the bottom left
    mouse.coords {
        x = 50 + 2*c.border_width,
        y = c.screen.geometry.height - 200
    }

    return true
end)

-- Ensure the move was correct, the snap to the top part of the screen
table.insert(steps, function()
    local c = client.get()[1]

    assert(c:geometry().x == 0)
    assert(c:geometry().y == c.screen.geometry.height - 250 - 2*c.border_width)
    assert(c:geometry().width  == 200)
    assert(c:geometry().height == 200)

    -- Should trigger the top snap
    mouse.coords {x = 600, y= 0}

    -- The snap is only applied on release
    root.fake_input("button_release",1)

    return true
end)

-- The client should now fill the top half of the screen
table.insert(steps, function()
    local c = client.get()[1]

    assert(c:geometry().x       == c.screen.workarea.x                                     )
    assert(c:geometry().y       == c.screen.workarea.y                                     )
    assert(c:geometry().width   == c.screen.workarea.width - 2*c.border_width              )
    assert(c:geometry().height  == math.ceil(c.screen.workarea.height/2 - 2*c.border_width))

    -- Snap to the top right
    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.top_right(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

-- The client should now fill the top right corner of the screen
table.insert(steps, function()
    local c = client.get()[1]

    assert(c:geometry().x       == math.ceil(c.screen.workarea.x+c.screen.workarea.width/2))
    assert(c:geometry().y       == c.screen.workarea.y                                     )
    assert(c:geometry().width   == math.ceil(c.screen.workarea.width/2 - 2*c.border_width) )
    assert(c:geometry().height  == math.ceil(c.screen.workarea.height/2 - 2*c.border_width))

    -- Snap to the top right
    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.right(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

-- The client should now fill the top right half of the screen
table.insert(steps, function()
    local c = client.get()[1]

    assert(c:geometry().x       == math.ceil(c.screen.workarea.x+c.screen.workarea.width/2))
    assert(c:geometry().y       == c.screen.workarea.y                                    )
    assert(c:geometry().width   == math.ceil(c.screen.workarea.width/2 - 2*c.border_width))
    assert(c:geometry().height  == c.screen.workarea.height - 2*c.border_width            )

    -- Snap to the top right
    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.bottom(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

-- The client should now fill the bottom half of the screen
table.insert(steps, function()
    local c = client.get()[1]

    assert(c:geometry().x      == c.screen.workarea.x                                     )
    assert(c:geometry().y      == math.ceil(c.screen.workarea.y+c.screen.workarea.height/2))
    assert(c:geometry().width  == c.screen.workarea.width - 2*c.border_width              )
    assert(c:geometry().height == math.ceil(c.screen.workarea.height/2 - 2*c.border_width))

    -- Snap to the top right
    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.bottom_left(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

-- The client should now fill the bottom left corner of the screen
table.insert(steps, function()
    local c = client.get()[1]

    assert(c:geometry().x       == c.screen.workarea.x                                     )
    assert(c:geometry().y       == math.ceil(c.screen.workarea.y+c.screen.workarea.height/2))
    assert(c:geometry().width   == math.ceil(c.screen.workarea.width/2 - 2*c.border_width) )
    assert(c:geometry().height  == math.ceil(c.screen.workarea.height/2 - 2*c.border_width))

    -- Snap to the top right
    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.left(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

-- The client should now fill the left half of the screen
table.insert(steps, function()
    local c = client.get()[1]

    assert(c:geometry().x      == c.screen.workarea.x                                     )
    assert(c:geometry().y      == c.screen.workarea.y                                     )
    assert(c:geometry().width  == math.ceil(c.screen.workarea.width/2 - 2*c.border_width) )
    assert(c:geometry().height == c.screen.workarea.height - 2*c.border_width             )

    -- Snap to the top right
    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.top_left(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

local cur_tag = nil

-- The client should now fill the top left corner of the screen
table.insert(steps, function()
    local c = client.get()[1]

    assert(c:geometry().x       == c.screen.workarea.x                                     )
    assert(c:geometry().y       == c.screen.workarea.y                                     )
    assert(c:geometry().width   == math.ceil(c.screen.workarea.width/2 - 2*c.border_width) )
    assert(c:geometry().height  == math.ceil(c.screen.workarea.height/2 - 2*c.border_width))

    -- Change the mode to test drag_to_tag
    amouse.drag_to_tag.enabled = true
    amouse.snap.edge_enabled   = false

    cur_tag = c.first_tag

    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.right(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

-- The tag should have changed
table.insert(steps, function()
    local c = client.get()[1]

    assert(c.first_tag       ~= cur_tag          )
    assert(c.first_tag.index == cur_tag.index + 1)

    -- Move it back
    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.left(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

-- The tag should now be the same as before
table.insert(steps, function()
    local c = client.get()[1]

    assert(c.first_tag == cur_tag)
    assert(c.first_tag.index == 1)

    -- Wrap
    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.left(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

-- The tag should now be the last
table.insert(steps, function()
    local c = client.get()[1]

    assert(c.first_tag.index == #c.screen.tags)

    -- Wrap back
    root.fake_input("button_press",1)
    amouse.client.move(c)
    placement.right(mouse, {honor_workarea=false})
    root.fake_input("button_release",1)

    return true
end)

-- The tag should now original one again
table.insert(steps, function()
    local c = client.get()[1]

    assert(c.first_tag == cur_tag)

    c:geometry {
        x      = 99,
        y      = 99,
        width  = 99,
        height = 99,
    }

    return true
end)

-- Test that odd number sized clients don't move by accident
for _=1, 15 do
    table.insert(steps, function()
        local c = client.get()[1]

        root.fake_input("button_press",1)
        amouse.client.move(c)
        root.fake_input("button_release",1)


        return true
    end)
end

table.insert(steps, function()
    local c = client.get()[1]
    root.fake_input("button_release",1)

    assert(c.x      == 99)
    assert(c.y      == 99)
    assert(c.width  == 99)
    assert(c.height == 99)

    return true
end)

table.insert(steps, function()
    for _, c in pairs(client.get()) do
        c:kill()
    end
    if #client.get() == 0 then
        test_client(nil, nil, nil, nil, true)
        return true
    end
end)

table.insert(steps, function()
    if #client.get() ~= 1 then
        return
    end

    local c = client.get()[1]
    local geo = c:geometry()
    local hints = c.size_hints
    assert(hints.height_inc == 200)
    assert(hints.width_inc == 200)
    assert(c:apply_size_hints(1, 1) == 0)

    c:geometry { width = 1, height = 50 }

    -- The above should be rejected, because it would make us resize the
    -- window size 0x0.
    assert(c:geometry().width == geo.width)
    assert(c:geometry().height == geo.height)
    return true
end)

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
