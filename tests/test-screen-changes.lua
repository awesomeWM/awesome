-- Tests for screen additions & removals

local runner = require("_runner")
local test_client = require("_client")
local naughty = require("naughty")
local max = require("awful.layout.suit.max")

local real_screen = screen[1]
local fake_screen = screen.fake_add(50, 50, 500, 500)
local test_client1, test_client2

local list_count = 0
screen.connect_signal("list", function()
    list_count = list_count + 1
end)

local steps = {
    -- Step 1: Set up some clients to experiment with and assign them as needed
    function(count)
        if count == 1 then  -- Setup.
            test_client()
            test_client()
        end
        local cls = client.get()
        if #cls == 2 then
            test_client1, test_client2 = cls[1], cls[2]
            test_client1.screen = real_screen
            test_client2.screen = fake_screen

            -- Use a tiled layout
            fake_screen.selected_tag.layout = max

            -- Display a notification on the screen-to-be-removed
            naughty.notify{ text = "test", screen = fake_screen }

            return true
        end
    end,

    -- Step 2: Move the screen
    function(count)
        if count == 1 then
            fake_screen:fake_resize(100, 110, 600, 610)
            return
        end

        -- Everything should be done by now
        local geom = test_client2:geometry()
        local bw = test_client2.border_width
        local ug = test_client2:tags()[1].gap
        assert(geom.x - 2*ug == 100, geom.x - 2*ug)
        assert(geom.y - 2*ug == 110, geom.y - 2*ug)
        assert(geom.width + 2*bw + 4*ug == 600, geom.width + 2*bw + 4*ug)
        assert(geom.height + 2*bw + 4*ug == 610, geom.height + 2*bw + 4*ug)

        local wb = fake_screen.mywibox
        assert(wb.screen == fake_screen, tostring(wb.screen) .. " ~= " .. tostring(fake_screen))
        assert(wb.x == 100, wb.x)
        assert(wb.y == 110, wb.y)
        assert(wb.width == 600, wb.width)

        -- Test screen order changes
        assert(list_count == 0)
        assert(screen[1] == real_screen)
        assert(screen[2] == fake_screen)
        real_screen:swap(fake_screen)
        assert(list_count == 1)
        assert(screen[2] == real_screen)
        assert(screen[1] == fake_screen)

        return true
    end,

    -- Step 3: Say goodbye to the screen
    function()
        local wb = fake_screen.mywibox
        fake_screen:fake_remove()

        -- Now that the screen is invalid, the wibox shouldn't refer to it any
        -- more
        assert(wb.screen ~= fake_screen)

        -- Wrap in a weak table to allow garbage collection
        fake_screen = setmetatable({ fake_screen }, { __mode = "v" })

        return true
    end,

    -- Step 4: Everything should now be on the main screen, the old screen
    -- should be garbage collectable
    function()
        assert(test_client1.screen == real_screen, test_client1.screen)
        assert(test_client2.screen == real_screen, test_client2.screen)

        collectgarbage("collect")
        if #fake_screen == 0 then
            return true
        end
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
