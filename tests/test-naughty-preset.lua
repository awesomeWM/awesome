local naughty = require("naughty")
local notification = require("naughty.notification")
require("ruled.notification"):_clear()

local steps = {}

local BAD_IDEA = "never do this in practice, compat only"

local notif = setmetatable({}, {__mode="v"})

screen[1]:split()

for _, legacy_preset in ipairs {true, false} do
    table.insert(steps, function()
        -- This will either take the legacy preset path
        -- or the ruled preset path.
        function naughty.get__has_preset_handler()
            return legacy_preset
        end

        local custom_preset = {
            bg   = "#00ff00",
            fg   = "#ff0000",
            text = BAD_IDEA
        }

        notif[1] = notification {
            preset = custom_preset,
        }

        assert(notif[1].bg == "#00ff00")
        assert(notif[1].message == BAD_IDEA)

        return true
    end)

    table.insert(steps, function()
        notif[1]:destroy()

        return true
    end)

    for s in screen do
        -- Make sure the screen doesn't cause a memory leak.
        table.insert(steps, function()
            collectgarbage("collect")

            if notif[1] then return end

            local custom_preset = {
                bg = "#0000ff",
                fg = "#ff0000",
                screen = s
            }

            notif[1] = notification {
                preset = custom_preset,
                title = "test",
            }

            assert(notif[1].bg == "#0000ff")

            assert(notif[1].screen == s)

            return true
        end)

        table.insert(steps, function()
            assert(notif[1].screen == s)
            notif[1]:destroy()

            return true
        end)

        table.insert(steps, function()
            collectgarbage("collect")

            if notif[1] then return end

            return true
        end)
    end
end

require("_runner").run_steps(steps)
