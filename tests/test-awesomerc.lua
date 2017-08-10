local awful = require("awful")
local hotkeys_widget = require("awful.hotkeys_popup").widget

-- luacheck: globals modkey

local old_c = nil


-- Get a tag and a client
local function get_c_and_t()
    client.focus = old_c or client.get()[1]
    local c  = client.focus

    local t = c.screen.selected_tag

    return c, t
end

local function num_pairs(container_table)
  local number_of_items = 0
  for _, _ in pairs(container_table) do
    number_of_items = number_of_items + 1
  end
  return number_of_items
end

local steps = {
    function(count)
        if count <= 5 then
            awful.spawn("xterm")
        elseif #client.get() >= 5 then
            local c, _ = get_c_and_t()
            old_c = c

            return true
        end
    end,

    -- Wait for the focus to change
    function()
        assert(old_c)

        -- Test layout

        --TODO use the key once the bug is fixed
        local l = old_c.screen.selected_tag.layout
        assert(l)

        --awful.key.execute({modkey}, " ")
        awful.layout.inc(1)

        assert(old_c.screen.selected_tag.layout ~= l)

        -- Test ontop

        assert(not old_c.ontop)
        awful.key.execute({modkey}, "t")
        awesome.sync()

        return true
    end,

    -- Ok, no now ontop should be true
    function()
        local _, t = get_c_and_t()

        -- Give awesome some time
        if not old_c.ontop then return nil end

        assert(old_c.ontop)

        -- Now, test the master_width_factor
        assert(t.master_width_factor == 0.5)

        awful.key.execute({modkey}, "l")
        awesome.sync()

        return true
    end,

    -- The master width factor should now be bigger
    function()
        local _, t = get_c_and_t()

        assert(t.master_width_factor == 0.55)

        -- Now, test the master_count
        assert(t.master_count == 1)

        awful.key.execute({modkey, "Shift"}, "h")
        awesome.sync()

        return true
    end,

    -- The number of master client should now be 2
    function()
        local _, t = get_c_and_t()

        assert(t.master_count == 2)

        -- Now, test the column_count
        assert(t.column_count == 1)

        awful.key.execute({modkey, "Control"}, "h")
        awful.key.execute({modkey, "Shift"  }, "l")
        awesome.sync()

        return true
    end,

    -- The number of columns should now be 2
    function()
        local _, t = get_c_and_t()

        assert(t.column_count == 2)

        -- Now, test the switching tag
        assert(t.index == 1)

        awful.key.execute({modkey, }, "Right")
        awesome.sync()

        return true
    end,

    -- The tag index should now be 2
    function()
        local tags = mouse.screen.tags
        --     local t = awful.screen.focused().selected_tag

        --     assert(t.index == 2)--FIXME

        assert(tags[1].index == 1)
        tags[1]:view_only()

        return true
    end,

    -- Before testing tags, lets make sure everything is still right
    function()
        local tags = mouse.screen.tags

        assert(tags[1].selected)

        local clients = mouse.screen.clients

        -- Make sure the clients are all on the same screen, they should be
        local c_scr = client.get()[1].screen

        for _, c in ipairs(client.get()) do
            assert(c_scr == c.screen)
        end

        -- Then this should be true
        assert(#clients == #client.get())

        assert(#mouse.screen.all_clients == #clients)

        assert(#mouse.screen.all_clients - #mouse.screen.hidden_clients == #clients)

        return true
    end,

    -- Now, test switching tags
    function()
        local tags = mouse.screen.tags
        local clients = mouse.screen.all_clients

        assert(#tags == 9)

        assert(#clients == 5)

        assert(mouse.screen.selected_tag == tags[1])

        for i=1, 9 do
            -- Check that assertion, because if it's false, the other assert()
            -- wont make any sense.
            assert(tags[i].index == i)
        end

        for i=1, 9 do
            tags[i]:view_only()
            assert(tags[i].selected)
            assert(#mouse.screen.selected_tags == 1)
        end

        tags[1]:view_only()

        return true
    end,

    -- Lets shift some clients around
    function()
        local tags = mouse.screen.tags

        -- Given all tags have been selected, the selection should be back on
        -- tags[1] and the client history should be kept
        assert(client.focus == old_c)

        --awful.key.execute({modkey, "Shift"  }, "#"..(9+i)) --FIXME
        client.focus:move_to_tag(tags[2])

        assert(not client.focus)

        return true
    end,

    -- The client should be on tag 5
    function()
        -- Confirm the move did happen
        local tags = mouse.screen.tags
        assert(tags[1].selected)
        assert(#old_c:tags() == 1)
        assert(old_c:tags()[1] ~= tags[1])
        assert(not old_c:tags()[1].selected)

        -- The focus should have changed by now, as the tag isn't visible
        assert(client.focus ~= old_c)

        assert(old_c:tags()[1] == tags[2])

        assert(#tags[2]:clients() == 1)
        assert(#tags[1]:clients() == 4)

        return true
    end,

    -- Hotkeys popup should be displayed and hidden
    function(count)
        local s = awful.screen.focused()
        local cached_wiboxes = hotkeys_widget.default_widget._cached_wiboxes

        if count == 1 then
            assert(num_pairs(cached_wiboxes) == 0)
            awful.key.execute({modkey}, "s")
            return nil

        elseif count == 2 then
            assert(num_pairs(cached_wiboxes) > 0)
            assert(num_pairs(cached_wiboxes[s]) == 1)
        end

        local hotkeys_wibox
        for _, widget in pairs(cached_wiboxes[s]) do
            hotkeys_wibox = widget.wibox
        end

        if count == 2 then
            assert(hotkeys_wibox ~= nil)
            assert(hotkeys_wibox.visible)
            -- Should disappear on anykey
            root.fake_input("key_press", "Super_L")

        elseif count == 3 then
            assert(not hotkeys_wibox.visible)
            root.fake_input("key_release", "Super_L")
            return true
        end
    end,

}

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
