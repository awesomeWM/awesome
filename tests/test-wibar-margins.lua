--- Test that fractional margins do not make a wibar grow.
--
-- The placement code adds the margins to the geometry it reads and removes
-- them from the geometry it writes. The drawin geometry is rounded to whole
-- pixels, so a fractional margin used to make the wibar one pixel taller every
-- time the attached placement ran, until the C stack overflowed (#4121).

local wibar = require("awful.wibar")

local steps = {}

-- A fractional margin, as half of an odd theme size gives. It is rounded to 11.
local margin, rounded = 10.5, 11

-- Remove the wibar from the default configuration, it would add its own size
-- to the margins of the wibar below.
screen.primary.mywibox:remove()
screen.primary.mywibox = nil

local height = 24
local sgeo   = screen.primary.geometry

local expected = {
    x      = sgeo.x + rounded,
    y      = sgeo.y + rounded,
    width  = sgeo.width - 2 * rounded,
    height = height,
}

local w

-- The top and bottom margins are on the same axis as the height of a top
-- wibar. Only the top one is set, so their sum is fractional.
table.insert(steps, function()
    -- Without the fix, this alone overflows the C stack.
    w = wibar {
        position = "top",
        screen   = screen.primary,
        height   = height,
        margins  = { top = margin, left = margin, right = margin },
    }

    return true
end)

-- The placement is re-applied every time the size changes, so keep checking
-- the geometry for a while to make sure it stays put.
for _=1, 3 do
    table.insert(steps, function()
        local geo = w:geometry()

        for _, key in ipairs {"x", "y", "width", "height"} do
            assert(geo[key] == expected[key], string.format(
                "regression: fractional margins changed the %s to %d, "..
                "expected %d", key, geo[key], expected[key]
            ))
        end

        return true
    end)
end

table.insert(steps, function()
    -- The strut covers the wibar and the margin above it.
    local expected_y = expected.y + expected.height

    assert(screen.primary.workarea.y == expected_y, string.format(
        "regression: the workarea starts at %d, expected %d",
        screen.primary.workarea.y, expected_y))

    return true
end)

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
