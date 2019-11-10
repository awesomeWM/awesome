--- Tests for _NET_CURRENT_DESKTOP

local runner = require("_runner")
local test_client = require("_client")
local awful = require("awful")
local gtable = require("gears.table")

local c
local s = screen[1]
local tags = s.tags

local function wait_for_current_desktop(tag)
    -- The X property has 0-based indicies
    local idx = gtable.hasitem(tags, tag) - 1
    return function()
        local file = io.popen("xprop -notype -root _NET_CURRENT_DESKTOP")
        local result = file:read("*all")
        file:close()

        -- Extract the value
        local value = string.sub(result, 24, -2)

        if tostring(idx) == value then
            return true
        end
        print(string.format("Got _NET_CURRENT_DESKTOP = '%s', expected %d", value, idx))
        return false
    end
end

local steps = {

    -- Set up the state we want
    function(count)
        if count == 1 then
            test_client()
            awful.tag.viewmore({ tags[3], tags[4], tags[5] }, s)
        end

        c = awful.client.visible()[1]

        if c then
            c:move_to_tag(tags[4])
            client.focus = c
            return true
        end
    end,
    wait_for_current_desktop(tags[4]),

    -- Move the client around
    function()
        c:move_to_tag(tags[5])
        return true
    end,
    wait_for_current_desktop(tags[5]),

    -- Move the client to a non-selected tag
    function()
        c:move_to_tag(tags[6])
        assert(client.focus == nil)
        return true
    end,
    wait_for_current_desktop(tags[3]),

    -- Move the client back
    function(count)
        if count == 1 then
            c:move_to_tag(tags[4])
            return
        end
        client.focus = c
        return true
    end,
    wait_for_current_desktop(tags[4]),

    -- Killing the client means the first selected tag counts
    function(count)
        if count == 1 then
            assert(c.active)
            c:kill()
            c = nil
            return
        end
        return true
    end,
    wait_for_current_desktop(tags[3]),

    -- Change tag selection
    function()
        tags[3].selected = false
        return true
    end,
    wait_for_current_desktop(tags[4]),

}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
