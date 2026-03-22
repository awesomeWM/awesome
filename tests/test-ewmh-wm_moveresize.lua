local test_client = require("_client")
local placement = require("awful.placement")
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

-- Testing utils
local signals_of_interest = {
    "request::mouse_move",
    "request::mouse_resize",
    "request::mouse_cancel"
}
local signal_buffer = {}

local function assert_signal(sig_name, asserts)
    local sig_data = signal_buffer[sig_name]
    assert(sig_data ~= nil, "expected signal: " .. sig_name)
    if asserts then
        asserts(unpack(sig_data))
    end
    signal_buffer = {}
end

local function assert_geometry(c, x, y, width, height)
    local geo = c:geometry()
    assert(geo.x      == x,      "expected x coord " .. x ..
        ", but it was " .. geo.x)
    assert(geo.y      == y,      "expected y coord " .. y ..
        ", but it was " .. geo.y)
    assert(geo.width  == width,  "expected width "   .. width ..
        ", but it was " .. geo.width)
    assert(geo.height == height, "expected height "  .. height ..
        ", but it was " .. geo.height)
end

local function reset_geometry(c)
    c:geometry { x = 200, y = 200, width = 400, height = 400 }
end

local function drag(x, y)
    mouse.coords {x = x, y = y}
end

local function click(x, y, button)
    drag(x, y)
    root.fake_input("button_press", button or 1)
end

local function unclick(button)
    root.fake_input("button_release", button or 1)
end

local resize_corners = {
    {corner = "right",        bx = -1, by = 0,  dx = 0, dy = 0, dw = 1,  dh = 0},
    {corner = "bottom_right", bx = -1, by = -1, dx = 0, dy = 0, dw = 1,  dh = 1},
    {corner = "bottom",       bx = 0,  by = -1, dx = 0, dy = 0, dw = 0,  dh = 1},
    {corner = "bottom_left",  bx = 1,  by = -1, dx = 1, dy = 0, dw = -1, dh = 1},
    {corner = "left",         bx = 1,  by = 0,  dx = 1, dy = 0, dw = -1, dh = 0},
    {corner = "top_left",     bx = 1,  by = 1,  dx = 1, dy = 1, dw = -1, dh = -1},
    {corner = "top",          bx = 0,  by = 1,  dx = 0, dy = 1, dw = 0,  dh = -1},
    {corner = "top_right",    bx = -1, by = 1,  dx = 0, dy = 1, dw = 1,  dh = -1},
}
local steps = {}

-- Mouse movement tests

table.insert(steps, function(count)
    if count == 1 then  -- Setup.
        test_client("foobar", "foobar", nil, nil, nil, {
            custom_titlebar = true
        })
    elseif #client.get() > 0 then
        local c = client.get()[1]

        reset_geometry(c)

        -- Attach signal handlers so that we can see which signals have been fired
        for _,sig_name in ipairs(signals_of_interest) do
            c:connect_signal(sig_name, function(_, ...)
                signal_buffer[sig_name] = {...}
                local _, args = ...
                if args and args.mouse_pos then
                    print("got signal " .. sig_name ..
                        " at (" .. args.mouse_pos.x .. ", " .. args.mouse_pos.y .. ")")
                else
                    print("got signal " .. sig_name)
                end
            end)
        end

        return true
    end
end)

table.insert(steps, function()
    local c = client.get()[1]

    -- GTK window may take some time to finish setting up
    -- Just repeat until the drag works
    if signal_buffer["request::mouse_move"] then return true end

    -- Just in case there is an accidental delayed geometry callback
    assert_geometry(c, 200, 200, 400, 400)

    -- Click the title bar and drag to start a mouse movement transaction
    -- This doesn't necessarily move the window yet!
    unclick()
    click(400, 220)
    drag(420, 220)
end)

table.insert(steps, function()
    assert_signal("request::mouse_move", function(context, args)
        assert(context          == "ewmh")
        assert(args.button      == 1)
        assert(args.mouse_pos.x == 400)
        assert(args.mouse_pos.y == 220)
    end)

    -- Actually move the window
    drag(500, 120)

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    assert_geometry(c, 300, 100, 400, 400)

    -- Release the mouse to end the movement
    unclick()

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    if mousegrabber.isrunning() then return end

    assert_geometry(c, 300, 100, 400, 400)

    -- Window should no longer follow mouse
    drag(300, 200)

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    assert_geometry(c, 300, 100, 400, 400)

    -- Start another mouse movement transaction
    click(500, 120)
    drag(520, 120)

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    if not signal_buffer["request::mouse_move"] then return end

    assert_signal("request::mouse_move", function(context, args)
        assert(context          == "ewmh")
        assert(args.button      == 1)
        assert(args.mouse_pos.x == 500)
        assert(args.mouse_pos.y == 120)
    end)

    assert_geometry(c, 300, 100, 400, 400)

    -- Move the window somewhere else
    drag(600, 170)

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    assert_geometry(c, 400, 150, 400, 400)

    -- Cancel the mouse movement with a signal
    c:emit_signal("request::mouse_cancel", "test")

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    assert_signal("request::mouse_cancel")
    assert_geometry(c, 400, 150, 400, 400)

    -- Window should no longer follow mouse
    drag(300, 200)

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    if mousegrabber.isrunning() then return end

    assert_geometry(c, 400, 150, 400, 400)

    -- Start a mouse movement transaction with the right mouse button
    -- (Note that the left button is still held from the previous test)
    click(380, 130, 2)
    c:emit_signal("request::mouse_move", "test", {
        button = 2,
        mouse_pos = { x = 380, y = 130 }
    })

    return true
end)

table.insert(steps, function()
    if not signal_buffer["request::mouse_move"] then return end

    assert_signal("request::mouse_move", function(context, args)
        assert(args.button      == 2)
        assert(args.mouse_pos.x == 380)
        assert(args.mouse_pos.y == 130)
    end)

    -- Move the window somewhere else and release the left mouse button
    drag(410, 270)
    unclick()

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    assert_geometry(c, 430, 290, 400, 400)

    -- The window should still be following the mouse
    assert(mousegrabber.isrunning())

    -- Move the window somewhere else and release the right mouse button
    drag(360, 230)
    unclick(2)

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    assert_geometry(c, 380, 250, 400, 400)

    -- Window should no longer follow the mouse
    drag(500, 100)

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    if mousegrabber.isrunning() then return end

    assert_geometry(c, 380, 250, 400, 400)

    -- Reset the client geometry for the next set of tests
    reset_geometry(c)

    return true
end)

-- Mouse resizing tests

local exp_x = 200
local exp_y = 200
local exp_w = 400
local exp_h = 400

local function assert_expected_geometry(c)
    assert_geometry(c, exp_x, exp_y, exp_w, exp_h)
end

local drag_amount = 50
local exp_mouse_x
local exp_mouse_y

for _,rc in ipairs(resize_corners) do
    table.insert(steps, function()
        local c = client.get()[1]

        assert_expected_geometry(c)

        local corner_pos = placement[rc.corner](mouse, {parent = c, pretend = true})
        exp_mouse_x = corner_pos.x + rc.bx * 3
        exp_mouse_y = corner_pos.y + rc.by * 3

        -- Click the edge of the window to start a mouse resizing transaction
        click(exp_mouse_x, exp_mouse_y)

        return true
    end)

    table.insert(steps, function()
        if not signal_buffer["request::mouse_resize"] then return end

        assert_signal("request::mouse_resize", function(context, args)
            assert(context          == "ewmh")
            assert(args.button      == 1)
            assert(args.mouse_pos.x == exp_mouse_x)
            assert(args.mouse_pos.y == exp_mouse_y)
        end)

        -- Resize the window
        drag(exp_mouse_x + drag_amount, exp_mouse_y + drag_amount)
        exp_x = exp_x + rc.dx * drag_amount
        exp_y = exp_y + rc.dy * drag_amount
        exp_w = exp_w + rc.dw * drag_amount
        exp_h = exp_h + rc.dh * drag_amount

        return true
    end)

    table.insert(steps, function()
        local c = client.get()[1]

        assert_expected_geometry(c)

        -- Release the mouse to end the resizing
        unclick()

        return true
    end)

    table.insert(steps, function()
        local c = client.get()[1]

        if mousegrabber.isrunning() then return end

        assert_expected_geometry(c)

        -- Window should no longer follow mouse (moving inwards)
        drag(exp_mouse_x + 30 * rc.bx, exp_mouse_y + 20 * rc.by)

        return true
    end)

    table.insert(steps, function()
        local c = client.get()[1]

        assert_expected_geometry(c)

        -- Window should no longer follow mouse (moving outwards)
        drag(exp_mouse_x - 20 * rc.bx, exp_mouse_y - 30 * rc.by)

        return true
    end)
end

table.insert(steps, function()
    local c = client.get()[1]

    -- Check that the final mouse movement didn't change the window
    assert_expected_geometry(c)

    -- Reset the client geometry for the next set of tests
    reset_geometry(c)

    return true
end)

-- Mouse resizing boundary clamp tests

for _,rc in ipairs(resize_corners) do
    table.insert(steps, function()
        local c = client.get()[1]

        local corner_pos = placement[rc.corner](mouse, {parent = c, pretend = true})
        exp_mouse_x = corner_pos.x + rc.bx * 3
        exp_mouse_y = corner_pos.y + rc.by * 3

        -- Click the edge of the window to start a mouse resizing transaction
        click(exp_mouse_x, exp_mouse_y)

        return true
    end)

    table.insert(steps, function()
        if not signal_buffer["request::mouse_resize"] then return end

        assert_signal("request::mouse_resize", function(context, args)
            assert(context          == "ewmh")
            assert(args.button      == 1)
            assert(args.mouse_pos.x == exp_mouse_x)
            assert(args.mouse_pos.y == exp_mouse_y)
        end)

        -- Resize the window beyond the opposite border
        drag(exp_mouse_x + 420 * rc.bx, exp_mouse_y + 420 * rc.by)
        unclick()

        return true
    end)

    table.insert(steps, function()
        local c = client.get()[1]

        if mousegrabber.isrunning() then return end

        -- The client should not have resized beyond the opposite border
        local geo = c:geometry()
        assert(geo.x >= 200, "client shrunk too far left: " .. geo.x)
        assert(geo.y >= 200, "client shrunk too far up: " .. geo.y)
        assert(geo.x + geo.width <= 600, "client shrunk too far right: " .. (geo.x + geo.width))
        assert(geo.y + geo.height <= 600, "client shrunk too far down: " .. (geo.y + geo.height))

        -- Reset the client geometry
        reset_geometry(c)

        return true
    end)

    table.insert(steps, function()
        -- Move the mouse out and back into the client area
        -- Otherwise, the next drag fails for some reason
        drag(0, 0)
        drag(400, 400)

        return true
    end)
end

-- Maximized client tests

table.insert(steps, function()
    local c = client.get()[1]

    -- Maximize the client
    c.maximized = true

    -- Try to start a mouse movement transaction
    c:emit_signal("request::mouse_move", "test", {
        button = 1,
        mouse_pos = { x = 80, y = 80 }
    })

    return true
end)

table.insert(steps, function()
    local c = client.get()[1]

    if not signal_buffer["request::mouse_move"] then return end

    assert_signal("request::mouse_move")

    -- The movement transaction should fail because the client is maximized
    assert(not mousegrabber.isrunning())

    -- Try to start a mouse resizing transaction
    c:emit_signal("request::mouse_resize", "test", {
        button = 1,
        mouse_pos = { x = 3, y = 3 }
    })

    return true
end)

table.insert(steps, function()
    if not signal_buffer["request::mouse_resize"] then return end

    assert_signal("request::mouse_resize")

    -- The resizing transaction should fail because the client is maximized
    assert(not mousegrabber.isrunning())

    return true
end)

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
