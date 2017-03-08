-- This test hit the client layout code paths to see if there is errors.
-- it doesn't check if the layout are correct.

local awful = require("awful")
local gtable = require("gears.table")

local first_layout = nil

local t = nil

local has_spawned = false

local steps = {

    -- Add enough clients
    function(count)
        if count <= 1 and not has_spawned then
            for _=1, 5 do awful.spawn("xterm") end
            has_spawned = true
        elseif #client.get() >= 5 then

            first_layout = client.focus:tags()[1].layout

            t = client.focus:tags()[1]

            return true
        end
    end,

}

local function next_layout()
    awful.layout.inc(1)

    assert(client.focus:tags()[1].layout ~= first_layout)

    return true
end

-- Test most properties for each layouts
local common_steps = {
    function()
        assert(#t:clients() == 5)

        t.master_count = 2

        return true
    end,
    function()
        t.master_count = 0

        return true
    end,
    function()
        t.master_count = 6 --more than #client.get(1)

        return true
    end,
    function()
        t.master_count = 1

        return true
    end,
    function()
        t.column_count = 2

        return true
    end,
    function()
        t.column_count = 6 --more than #client.get(1)

        return true
    end,
    function()
        t.column_count = 1

        return true
    end,
    function()
        t.master_fill_policy = t.master_fill_policy == "master_width_factor" and
        "expand" or "master_width_factor"

        return true
    end,
    function()
        t.master_width_factor = 0.75

        return true
    end,
    function()
        t.master_width_factor = 0

        return true
    end,
    function()
        t.master_width_factor = 1

        return true
    end,
    function()
        t.master_width_factor = 0.5

        return true
    end,
    function()
        t.gap = t.gap == 0 and 5 or 0

        return true
    end,
}

local first = false
for _ in ipairs(awful.layout.layouts) do
    if not first then
        first = true
    else
        gtable.merge(steps, {next_layout})
    end

    gtable.merge(steps, common_steps)
end

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
