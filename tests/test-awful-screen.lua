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
    end
}

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
