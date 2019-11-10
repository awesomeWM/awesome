local awful = require("awful")
local test_client = require("_client")

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
            local c  = client.focus
            -- local c2 = client.get()[2]

            c.foo = "bar"

            -- Check if the property system works
            assert(c.foo == "bar")
            assert(c.foo == awful.client.property.get(c, "foo"))

            -- Test jumpto

            --FIXME doesn't work
            -- c2:jump_to()
            -- assert(client.focus == c2)
            -- awful.client.jumpto(c)
            -- assert(client.focus == c)

            -- Test relative_move
            c.size_hints_honor = false
            c:geometry {x=0,y=0,width=50,height=50}
            c:relative_move( 100, 100, 50, 50 )
            for _,v in pairs(c:geometry()) do
                assert(v == 100)
            end
            c:relative_move(-25, -25, -25, -25)
            for _,v in pairs(c:geometry()) do
                assert(v == 75)
            end

            -- Test move_to_tag

            local t  = mouse.screen.tags[1]
            local t2 = mouse.screen.tags[2]

            c:tags{t}
            assert(c:tags()[1] == t)
            c:move_to_tag(t2)
            assert(c:tags()[1] == t2)
            c:move_to_tag(t)
            assert(c:tags()[1] == t)

            -- Test toggle_tag

            c:tags{t}
            c:toggle_tag(t2)
            assert(c:tags()[1] == t2 or c:tags()[2] == t2)
            c:toggle_tag(t2)
            assert(c:tags()[1] == t and c:tags()[1] ~= t2 and c:tags()[2] == nil)

            -- Test floating
            assert(c.floating ~= nil and type(c.floating) == "boolean")
            c.floating = true
            assert(c.floating)
            c.floating = false
            assert(not c.floating)

            return true
        end
    end,

    -- Test implicitly floating state: Even though a client way not explicitly
    -- made floating, it could still end up being floating
    function()
        local c = client.get()[1]
        assert(c ~= nil)

        awful.client.property.set(c, "floating", nil)
        assert(not c.floating)
        assert(not c.maximized)

        local signal_count = 0
        c:connect_signal("property::floating", function()
            signal_count = signal_count + 1
        end)

        c.maximized = true
        assert(c.floating)
        assert(signal_count == 1)

        c.fullscreen = true
        assert(c.floating)
        assert(signal_count == 1)

        c.maximized = false
        assert(c.floating)
        assert(signal_count == 1)

        c.fullscreen = false
        assert(c.floating == false)
        assert(signal_count == 2)

        return true
    end,

    -- Ensure that window factor is ignored on maximized clients
    function()
        local c = client.get()[1]
        assert(c ~= nil)

        local signal_count = 0
        c:connect_signal("property::windowfact", function()
            signal_count = signal_count + 1
        end)

        c.maximized = true

        awful.client.incwfact(0.1, c)
        awful.client.setwfact(0.5, c)
        assert(signal_count == 0)

        return true
    end,
}

local original_count, c1, c2 = 0

-- Check request::activate
table.insert(steps, function()
    c1, c2 = client.get()[1], client.get()[2]

    -- This should still be the case
    assert(c1.active)

    c2:emit_signal("request::activate", "i_said_so")

    return true
end)

-- Check if writing a focus stealing filter works.
table.insert(steps, function()
    -- This should still be the case
    assert(c2.active)

    original_count = #awful.ewmh.generic_activate_filters

    awful.ewmh.add_activate_filter(function(c)
        if c == c1 then return false end
    end)

    c1:emit_signal("request::activate", "i_said_so")

    return true
end)

table.insert(steps, function()
    -- The request should have been denied
    assert(c2.active)

    -- Test the remove function
    awful.ewmh.remove_activate_filter(function() end)

    awful.ewmh.add_activate_filter(awful.ewmh.generic_activate_filters[1])

    awful.ewmh.remove_activate_filter(awful.ewmh.generic_activate_filters[1])

    assert(original_count == #awful.ewmh.generic_activate_filters)

    c1:emit_signal("request::activate", "i_said_so")

    return c1.active
end)

local has_error

-- Disable awful.screen.preferred(c)
awful.rules.rules[1].properties.screen = nil

table.insert(steps, function()
    -- Make sure there is no extra callbacks that causes double screen changes
    -- (regress #1028 #1052)
    client.connect_signal("property::screen",function(c)
        if c.class and c.class ~= "screen"..c.screen.index then
            has_error = table.concat {
                "An unexpected screen change did occur\n",
                debug.traceback(),"\n",
                c.class, " Screen: ",c.screen.index
            }
        end
    end)

    return true
end)



local multi_screen_steps = {}

-- Add a test client on each screen.
table.insert(multi_screen_steps, function()
    has_error = false
    for i=1, screen.count() do
        local s = screen[i]
        test_client("screen"..i, nil, {
                    screen = s,
                })
    end

    return true
end)

-- Test if the clients have been placed on the right screen
table.insert(multi_screen_steps, function()
    if #client.get() ~= screen.count() then return end

    if has_error then print(has_error) end
    assert(not has_error)

    for i=1, screen.count() do
        local s = screen[i]

        -- Just in case
        for _, t in ipairs(s.selected_tags) do
            assert(t.screen == s)
        end

        local t = s.selected_tag
        assert(t and t.screen == s)

        -- In case the next step has failed, find what went wrong
        for _, c in ipairs(t:clients()) do
            -- This is _supposed_ to be impossible, but can be forced by buggy
            -- lua code.
            if c.screen ~= t.screen then
                local tags = c:tags()
                for _, t2 in ipairs(tags) do
                    assert(t2.screen == c.screen)
                end

                assert(false)
            end
            assert(c.screen.index == i)
        end

        -- Check that the right client is placed, the loop above will trigger
        -- asserts already, but in case the tag has more than 1 client and
        -- everything is messed up, those asserts are still necessary.
        assert(#t:clients() == 1)
        assert(t:clients()[1].class == "screen"..i)
        assert(t:clients()[1].screen == s)

        -- Make sure it stays where it belongs
        awful.placement.maximize(t:clients()[1])
        assert(t:clients()[1] and t:clients()[1].screen == t.screen)
    end

    return true
end)

-- Test the `new_tag` rule
table.insert(multi_screen_steps, function()
    for _, c in ipairs(client.get()) do
        c:kill()
    end
    if #client.get() == 0 then
        return true
    end
end)

table.insert(multi_screen_steps, function()
    for i=1, screen.count() do
        local s = screen[i]
        test_client("screen"..i, nil, {
                    new_tag = {
                        name = "NEW_AT_"..i,
                        screen = s,
                    }
                })
    end

    return true
end)

table.insert(multi_screen_steps, function()
    if #client.get() ~= screen.count() then return end

    for _, c in ipairs(client.get()) do
        assert(#c:tags() == 1)
        assert(c.first_tag.name == "NEW_AT_"..c.screen.index)
    end

    -- Kill the client
    for _, c in ipairs(client.get()) do
        c:kill()
    end
    return true
end)

table.insert(multi_screen_steps, function()
    if #client.get() == 0 then
        return true
    end
end)

table.insert(multi_screen_steps, function()

    if screen.count() < 2 then return true end

    -- Now, add client where the target tag and screen don't match
    test_client("test_tag1", nil, {
                tag    = screen[2].tags[2],
                screen = screen[1],
            })

    -- Same as previous, but switched
    test_client("test_tag2", nil, {
                tag    = screen[1].tags[5],
                screen = screen[2],
            })

    -- Add a client with multiple tags on the same screen, but not c.screen
    test_client("test_tags1", nil, {
                tags   = { screen[1].tags[3], screen[1].tags[4] },
                screen = screen[2],
            })

    -- Identical, but using the tag names
    test_client("test_tags2", nil, {
                tags   = { "3", "4" },
                screen = screen[2],
            })

    -- Also test tags, but with an invalid screen array
    test_client("test_tags3", nil, {
                tags   = { screen[2].tags[3], screen[1].tags[4] },
                screen = screen[1],
            })

    -- Another test for tags, but with no matching names
    test_client("test_tags4", nil, {
                tags   = { "foobar", "bobcat" },
                screen = screen[1],
            })

    return true
end)


table.insert(multi_screen_steps, function()
    if screen.count() < 2 then return true end
    if #client.get() ~= 6 then return end

    local c_by_class = {}

    for _, c in ipairs(client.get()) do
        c_by_class[c.class] = c
    end

    assert(c_by_class["test_tag1"].screen == screen[2])
    assert(#c_by_class["test_tag1"]:tags() == 1)
    assert(c_by_class["test_tag1"]:tags()[1] == screen[2].tags[2])

    assert(c_by_class["test_tag2"].screen == screen[1])
    assert(#c_by_class["test_tag2"]:tags() == 1)
    assert(c_by_class["test_tag2"]:tags()[1] == screen[1].tags[5])

    assert(c_by_class["test_tags1"].screen == screen[1])
    assert(#c_by_class["test_tags1"]:tags() == 2)

    assert(c_by_class["test_tags2"].screen == screen[2])
    assert(#c_by_class["test_tags2"]:tags() == 2)

    assert(c_by_class["test_tags3"].screen == screen[2])
    assert(#c_by_class["test_tags3"]:tags() == 1)

    assert(c_by_class["test_tags4"].screen == screen[1])
    assert(#c_by_class["test_tags4"]:tags() == 1)
    assert(c_by_class["test_tags4"]:tags()[1] == screen[1].selected_tag)

    return true
end)

local ms = require("_multi_screen")
ms.disable_wibox()
ms(steps, multi_screen_steps)

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
