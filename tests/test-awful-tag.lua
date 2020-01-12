local awful = require("awful")
local gtable = require("gears.table")
local gdebug = require("gears.debug")
local beautiful = require("beautiful")

local function check_order()
    local tags = mouse.screen.tags

    for k, v in ipairs(tags) do
        assert(k == v.index)
    end
end

local has_spawned = false
local steps = {

    function(count)

        if count <= 1 and not has_spawned and #client.get() < 2 then
            awful.spawn("xterm")
            awful.spawn("xterm")
            has_spawned = true
        elseif #client.get() >= 2 then

            -- Test move, swap and index

            local tags = mouse.screen.tags

            assert(#mouse.screen.tags == 9)

            check_order()

            tags[7].index = 9
            assert(tags[7].index == 9)

            check_order()

            tags[7].index = 4
            assert(tags[7].index == 4)

            check_order()

            tags[7].index = 5
            assert(tags[7].index == 5)

            check_order()

            tags[1]:swap(tags[3])

            check_order()

            assert(tags[1].index == 3)
            assert(tags[3].index == 1)

            check_order()

            tags[3]:swap(tags[1])

            assert(tags[3].index == 3)
            assert(tags[1].index == 1)

            check_order()

            -- Test add, icon and delete

            client.focus = client.get()[1]
            local c  = client.focus
            assert(c and c.active)
            assert(beautiful.awesome_icon)

            local t = awful.tag.add("Test", {clients={c}, icon = beautiful.awesome_icon})
            assert(t.layout == awful.layout.suit.floating)

            check_order()

            local found = false

            tags = mouse.screen.tags

            assert(#tags == 10)

            for _, v in ipairs(tags) do
                if t == v then
                    found = true
                    break
                end
            end

            assert(found)

            assert(t:clients()[1] == c)
            assert(c:tags()[2] == t)
            assert(t.icon == beautiful.awesome_icon)

            t:delete()

            tags = mouse.screen.tags
            assert(#tags == 9)

            found = false

            for _, v in ipairs(tags) do
                if t == v then
                    found = true
                    break
                end
            end

            assert(not found)

            -- Test selected tags, view only and selected()

            t = tags[2]

            assert(not t.selected)

            assert(t.screen.selected_tag == tags[1])

            t:view_only()

            assert(t.selected)

            assert(not tags[1].selected)

            assert(#t.screen.selected_tags == 1)

            return true
        end
    end,

    -- Test the custom layout list
    function()
        local t = mouse.screen.tags[9]
        assert(t)

        -- Test that the global_layouts is the default
        local global_layouts = awful.layout.layouts

        local t_layouts = t.layouts
        local count = #t_layouts
        local original = gtable.clone(t_layouts, false)
        assert(t_layouts and global_layouts)

        assert(#global_layouts == #t_layouts)
        for idx, l in ipairs(global_layouts) do
            assert(t_layouts[idx] == l)
        end

        -- Make sure the list isn't forked
        t.layout = global_layouts[3]
        assert(t_layouts == t.layouts)

        -- Add a dummy layout
        t.layout = {name = "fake1", arrange=function() end}
        assert(#t.layouts == #global_layouts+1)

        -- Make sure adding global layouts still work
        table.insert(global_layouts, {name = "fake2", arrange=function() end})
        assert(#t.layouts == count+2)

        -- Test that the global list is forked when there is no other
        -- options.
        table.insert(global_layouts, function()
            return {name = "fake2", arrange=function() end}
        end)
        assert(#t.layouts == count+3)
        assert(#original == #global_layouts-2)
        assert(not t._layouts)

        t.layout = global_layouts[#global_layouts]

        assert(t._layouts)
        assert(#t._custom_layouts+#global_layouts == #t.layouts)
        assert(#t.layouts == count+3, "Expected "..(count+3).." got "..#t.layouts)

        -- Create a custom list of layouts
        t.layouts = {
            awful.layout.suit.fair,
            awful.layout.suit.fair.horizontal,
        }

        -- 3 because the current layout was added
        assert(#t.layouts == 3)

        t.layout = {name = "fake3", arrange=function() end}
        assert(#t.layouts == 4)

        -- Test the layout template and stateful layouts
        t.layouts = {
            t.layout,
            function() return {name = "fake4", arrange=function() end} end
        }
        assert(#t.layouts == 2)

        t.layout = t.layouts[2]
        assert(#t.layouts == 2)

        return true
    end,

    -- Test adding and removing layouts.
    function()
        local t = mouse.screen.tags[9]
        local count = #t.layouts
        t:append_layout(awful.layout.suit.floating)
        assert(#t.layouts == count + 1)

        t:append_layouts({
            awful.layout.suit.floating,
            awful.layout.suit.floating,
        })

        assert(#t.layouts == count + 3)

        t:remove_layout(awful.layout.suit.floating)
        assert(#t.layouts == count)

        return true
    end
}

local multi_screen_steps = {}

-- Clear the state (this step will be called once per disposition)
table.insert(multi_screen_steps, function()
    if #client.get() == 0 then
        -- Remove all existing tags created by the rc.lua callback
        for s in screen do
            while #s.tags > 0 do
                local t = s.tags[1]
                -- Test #1051 against regressions
                assert(t.index == 1)

                assert(t.screen == s)

                t:delete()
                assert(not t.activated)
                assert(t.screen == nil)
            end
        end

        return true
    end

    for _, c in ipairs(client.get()) do
        c:kill()
    end
end)

-- Add a bunch of tags on each screens.
table.insert(multi_screen_steps, function()
    for i=1, screen.count() do
        local s = screen[i]
        -- Add 5 tags per screen
        for idx=1, 5 do
            awful.tag.add("S"..i..":"..idx,{screen = s})
        end
    end

    -- Check if everything adds up
    for i=1, screen.count() do
        local tags = screen[i].tags
        assert(#tags == 5)

        for k, t in ipairs(tags) do
            assert(t.index == k)
            assert(t.name == "S"..i..":"..k)
        end
    end

    -- Select a tag on each screen
    for i=1, screen.count() do
        screen[i].tags[1].selected = true
    end

    return true
end)

local function check_tag_indexes()
    for s in screen do
        for i, t in ipairs(s.tags) do
            assert(t.index  == i)
            assert(t.screen == s)
        end
    end
end

-- Move tags across screens (without clients)
table.insert(multi_screen_steps, function()
    -- This screen disposition cannot be used for this test, move on
    if screen.count() < 2 then return true end

    local s1, s2 = screen[1], screen[2]

    local old_count1, old_count2 = #s1.tags, #s2.tags

    local t = awful.tag.add("TEST", {screen=s1})

    -- Check the current state
    assert( t.index               == old_count1+1 )
    assert( t.screen              == s1           )
    assert( #s1.tags              == old_count1+1 )
    assert( s1.tags[old_count1+1] == t            )
    assert( #s2.tags              == old_count2   )

    -- Move to another index
    local new_index = 3
    t.index = new_index
    assert(t.index == new_index)
    check_tag_indexes()

    -- Move the tag to screen 2
    t.screen = s2
    assert(t.screen               == s2           )
    assert(#s1.tags               == old_count1   )
    assert(#s2.tags               == old_count2+1 )
    assert( s2.tags[old_count2+1] == t            )
    assert( t.index               == old_count2+1 )
    check_tag_indexes()

    -- Move to another index
    t.index = new_index
    assert(t.index == new_index)
    check_tag_indexes()

    -- Delete it
    t:delete()
    check_tag_indexes()

    return true
end)

local ms = require("_multi_screen")
ms.disable_wibox()
ms(steps, multi_screen_steps)

-- Check deprecation.
table.insert(steps, function()
    assert(#awful.layout.layouts > 0)

    local called1, called2 = false, false
    gdebug.deprecate     = function() called1 = true end
    gdebug.print_warning = function() called2 = true end

    awful.layout.layouts = {}

    assert(called2)
    assert(not called1)
    assert(#awful.layout.layouts == 0)

    -- Test the random property setter.
    awful.layout.foo = "bar"
    assert(awful.layout.foo == "bar")

    return true
end)

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
