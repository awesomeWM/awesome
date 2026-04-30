-- Ensure the wibox and internal `drawin` objects don't leak.
local runner = require("_runner")
-- local wibox = require("wibox")

local steps = {}

-- local wiboxes   = setmetatable({}, {__mode = "v"})
local drawins   = setmetatable({}, {__mode = "v"})
local keys      = setmetatable({}, {__mode = "v"})
local drawables = {}

-- Establish a baseline that it should work using the `key` objects.
table.insert(steps, function()
    for _=1, 5 do
        table.insert(keys, key { key="#1" })
    end

    return true
end)

table.insert(steps, function()
    for _=1, 5 do
        collectgarbage("collect")
    end

    if #keys > 0 then return end

    return true
end)

-- Add some invisible wiboxes.
--
-- Since they are not visible, they have zero internal references and should be
-- GCed without any issue.
table.insert(steps, function()
    for _=1, 5 do
        local d = drawin {
            visible = false,
            x       = 0,
            y       = 0,
            width   = 100,
            height  = 100,
        }

        for _=1, 5 do
            d.visible = true
            d.visible = false
        end

        table.insert(drawins, d)
        table.insert(drawables, d.drawable)
    end

    return true
end)

table.insert(steps, function()
    for _, d in ipairs(drawins) do
        d.visible = false
    end

    return true
end)

table.insert(steps, function()
    for _=1, 5 do
        collectgarbage("collect")
    end

    if #drawins > 0 then return end

    -- Check if this is a no-op when the drawin is wiped.
    for _, d in ipairs(drawables) do
        d:refresh()
    end

    setmetatable(drawables, {__mode = "v"})

    for _=1, 5 do
        collectgarbage("collect")
    end

    return true
end)

table.insert(steps, function()
    if #drawables > 0 then return end

    return true
end)

-- This is flacky and I don't know why. `visible = true` removes the ref from
-- `LUAA_OBJECT_REGISTRY_KEY`, but the GC doesn't seem to agree with that.

-- Try again, but make the wibox visible this time.
--
-- This creates an internal reference and the weak table should hold them.
-- table.insert(steps, function()
--     for i=1, 5 do
--         local d = drawin {
--             visible = true,
--             x       = 0,
--             y       = 0,
--             width   = 100,
--             height  = 100,
--         }
--         table.insert(drawins, d)
--     end
--
--     for _=1, 5 do
--         collectgarbage("collect")
--     end
--
--     assert(#drawins == 5)
--
--     return true
-- end)
--
-- table.insert(steps, function()
--     for _, d in ipairs(drawins) do
--         -- This calls `unref`.
--         d.visible = false
--     end
--
--     return true
-- end)
--
-- table.insert(steps, function()
--     for _=1, 5 do
--         collectgarbage("collect")
--     end
--
--     while #drawins > 0 do return end
--
--     return true
-- end)

-- table.insert(steps, function()
--     wibox = require("wibox")
--
--     for i=1, 5 do
--         local w = wibox {
--             visible = true,
--             x       = 0,
--             y       = 0,
--             width   = 100,
--             height  = 100,
--         }
--         table.insert(wiboxes, w)
--         table.insert(drawins, w.drawin)
--     end
--
--     return true
-- end)
--
-- table.insert(steps, function()
--     for _, w in ipairs(wiboxes) do
--         w.visible = false
--     end
--
--     return true
-- end)
--
-- table.insert(steps, function()
--     for _=1, 5 do
--         collectgarbage("collect")
--     end
--
--     while #wiboxes > 0 do return end
--     while #drawins > 0 do return end
--
--     return true
-- end)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
