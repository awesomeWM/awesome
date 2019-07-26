local awful = require("awful")

local has_spawned = false
local steps = {
    function(count)
        if count <= 1 and not has_spawned and #client.get() < 2 then
            awful.spawn("xterm")
            awful.spawn("xterm")
            has_spawned = true

        elseif #client.get() >= 2 then
            -- Test properties
            client.focus = client.get()[1]

            local s = mouse.screen
            assert(s)
            assert(s == screen[s])

            -- Test padding
            s.padding = 42
            local counter = 0
            for _, v in pairs(s.padding) do
                assert(v == 42)
                counter = counter + 1
            end
            assert(counter == 4)

            s.padding = {
                            left   = 1337,
                            right  = 1337,
                            top    = 1337,
                            bottom = 1337,
                        }
            counter = 0
            for _, v in pairs(s.padding) do
                assert(v == 1337)
                counter = counter + 1
            end
            assert(counter == 4)

            counter = 0
            for _, v in pairs(s.padding) do
                assert(v == 1337)
                counter = counter + 1
            end
            assert(counter == 4)

            -- Test square distance
            assert(s:get_square_distance(9999, 9999))

            -- Test count
            counter = 0
            for _ in screen do
                counter = counter + 1
            end

            assert(screen.count() == counter)

            counter = 0
            awful.screen.connect_for_each_screen(function()
                counter = counter + 1
            end)
            assert(screen.count() == counter)

            return true
        end
    end,
    -- Make sure the error code still works when all screens are gone.
    function()
        while screen.count() > 0 do
            -- Before removing them, check if `name` works.
            assert(screen[1].name == "screen1")
            screen[1].name = "foo"
            assert(screen[1].name == "foo")
            assert(screen.foo == screen[1])

            screen[1]:fake_remove()
        end

        -- Don't make the test fail.
        local called = false
        require("gears.debug").print_warning = function() called = true end

        -- Cause an error in a protected call!
        awesome.emit_signal("debug::error", "err")

        assert(called)

        return true
    end,
    -- Test the `automatic_factory` getter and setter.
    function()
        assert(type(screen.automatic_factory) == "boolean")
        local orig = screen.automatic_factory
        screen.automatic_factory = not orig
        assert(screen.automatic_factory ~= orig)
        assert(type(screen.automatic_factory) == "boolean")
        screen.automatic_factory = not screen.automatic_factory
        assert(screen.automatic_factory == orig)
        return true
    end
}

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
