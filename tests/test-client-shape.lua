-- Test that client shapes work

local runner = require("_runner")
local spawn = require("awful.spawn")
local surface = require("gears.surface")
local wibox = require("wibox")

local steps = {
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

        -- xeyes sets bounding shape
        assert(surface.load_silently(c.client_shape_bounding, false))
        assert(surface.load_silently(c.shape_bounding, false))

        -- xeyes doesn't set clip shape
        assert(not surface.load_silently(c.client_shape_clip, false))
        assert(not surface.load_silently(c.shape_clip, false))

        -- xeyes doesn't set input shape, but we cannot query it, so we act as if
        -- it did. See xwindow_get_shape() for details.
        assert(surface.load_silently(c.client_shape_input, false))
        assert(surface.load_silently(c.shape_input, false))

        return true
    end,

    function()
        client.get()[1]:kill()
        return true
    end
}

local wb = nil
local wb_ready = false
local wb_clicks = 0
table.insert(steps, function(count)
    if count == 1 then
        local widget = wibox.widget.base.make_widget()
        function widget.draw()
            wb_ready = true
        end
        local geometry = {
            x = 0,
            y = 0,
            width = 500,
            height = 500,
        }
        wb = wibox {
            widget = widget,
        }
        wb:geometry(geometry)
        wb.visible = true
        wb.bg = "#888888FF"
        wb:connect_signal("button::press", function()
            wb_clicks = wb_clicks + 1
        end)
    elseif wb_ready then
        return true
    end
end)

local app_ready = false
local app_exited = false
local app_clicks = 0
table.insert(steps, function(count)
    if count == 1 then
        spawn.with_line_callback(
            "./test-client-shape-input 100 100 300 300",
            {
                stdout = function(line)
                    if line == "Ready" then
                        app_ready = true
                    end
                    if line:match("Click.*") then
                        app_clicks = app_clicks + 1
                    end
                end,
                stderr = function(line)
                    print("Test app stderr: " .. line)
                end,
                exit = function()
                    app_exited = true
                end
            })
    elseif (app_ready) then
        return true
    end
end)

local function click(x, y, expect_hit)
    table.insert(steps, function()
        wb_clicks = 0
        app_clicks = 0

        mouse.coords{x=x, y=y}
        assert(mouse.coords().x == x and mouse.coords().y == y)
        root.fake_input("button_press", 1)
        root.fake_input("button_release", 1)
        awesome.sync()

        return true
    end)

    table.insert(steps, function()
        assert(not app_exited)
        if wb_clicks + app_clicks > 0 then
            local pos = "(" .. tostring(x) .. ", " .. tostring(y) .. ")"
            if expect_hit then
                assert(wb_clicks == 0, "Wibox should not be clicked at " .. pos)
                assert(app_clicks == 1, "App should be clicked at " .. pos)
            else
                assert(wb_clicks == 1, "Wibox should be clicked at " .. pos)
                assert(app_clicks == 0, "App should not be clicked at " .. pos)
            end
            return true
        end
    end)
end

click( 40, 200, false)  -- outside app
click(140, 200, false)  -- inside app, not clickable
click(240, 200, false)  -- inside app, not clickable
click(340, 200, true)   -- inside app, clickable
click(440, 200, false)  -- outside app

table.insert(steps, function(count)
    if count == 1 then
        assert(not app_exited)
        wb = nil
        client.get()[1]:kill()
    elseif app_exited then
        return true
    end
end)

runner.run_steps(steps, { wait_per_step = 999999})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
