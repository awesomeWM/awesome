require("awful._compat")

local runner      = require( "_runner"         )
local placement   = require( "awful.placement" )
local gtable      = require( "gears.table"     )
local test_client = require( "_client"         )
local akeyboard   = require( "awful.keyboard"  )
local amouse      = require( "awful.mouse"     )

local module = {
    key    = require( "awful.key"    ),
    button = require( "awful.button" )
}

local steps = {}

local second = {
    key    = { "a", "b", "c", "d" },
    button = {  1 ,  2 ,  3 ,  4  },
}

local function send_events(event_type, step)
    root.fake_input("key_press"  , "Alt_L")
    root.fake_input(event_type.."_press"  , second[event_type][step])
    root.fake_input(event_type.."_release", second[event_type][step])
    root.fake_input("key_release", "Alt_L")
end

-- Test the shared API between the keys and buttons
for _, type_name in ipairs { "key", "button" } do
    local objects = {}

    local pressed, released = {}, {}

    table.insert(steps, function()
        objects[1] = module[type_name](
            {"Mod1"}, second[type_name][1], function() pressed[1] = true end, function() released[1] = true end
        )

        assert(objects[1].has_root_binding == false)
        assert(not pressed [1])
        assert(not released[1])

        root["_append_"..type_name](objects[1])

        -- It is async, the result need to be verified later
        return true
    end)

    table.insert(steps, function()
        assert(objects[1].has_root_binding)

        -- Test "artificial" execution
        objects[1]:trigger()

        assert(pressed [1])
        assert(released[1])

        -- Test adding and removing in the same iteration
        root["_remove_"..type_name](objects[1])

        assert(second[type_name][2])

        -- Use the multiple parameters syntax.
        objects[2] = module[type_name](
            {"Mod1"}, second[type_name][2], function() pressed[2] = true end, function() released[2] = true end
        )

        root["_append_"..type_name](objects[2])

        return true
    end)

    table.insert(steps, function()
        assert(objects[2].has_root_binding == true )
        assert(objects[1].has_root_binding == false)

        -- Make sure the cursor is not over the wibar
        placement.centered(mouse)

        assert(second[type_name][2])
        send_events(type_name, 2)

        return true
    end)

    -- Wait until the events are registered.
    table.insert(steps, function()
        if (not pressed[2]) or (not released[2]) then return end

        -- Spawn a client.
        test_client("myclient")

        return true
    end)

    --FIXME it works when manually tested, but automated testing is too flacky
    -- to enable...
    if type_name == "key" then
        table.insert(steps, function()
            if #mouse.screen.clients ~= 1 then return end

            placement.maximize(mouse.screen.clients[1])

            return true
        end)

        local o1, o2 = nil

        table.insert(steps, function()
            local c = mouse.screen.clients[1]

            -- This time, use the `args` syntax.
            local args = {
                modifiers   = {"Mod1"},
                on_press    = function() pressed [3] = true end,
                on_release  = function() released[3] = true end
            }

            args[type_name] = second[type_name][3]

            -- This time, use the `args` syntax.
            o1 = module[type_name](args)

            -- Test the old API.
            c[type_name.."s"](c, gtable.join(o1))

            return true
        end)

        -- This wont work until the client buttons/keys are updated, there is no
        -- way to know ahead of time.
        table.insert(steps, function(count)

            if count < 5 then awesome.sync(); return end

            send_events(type_name, 3)

            return true
        end)

        table.insert(steps, function()
            if (not pressed[3]) or (not released[3]) then return end

            local c = mouse.screen.clients[1]

            -- Unset the events to make sure keys can be removed.
            pressed[3], released[3] = false, false

            o2 = module[type_name](
                {"Mod1"}, second[type_name][4], function() pressed[4] = true end, function() released[4] = true end
            )

            -- Test the new API
            c[type_name.."s"] = {o2}

            return true
        end)

        table.insert(steps, function(count)
            if count < 5 then awesome.sync(); return end

            send_events(type_name, 3)
            send_events(type_name, 4)

            return true
        end)

        table.insert(steps, function()
            if (not pressed[4]) or (not released[4]) then return end

            assert(not pressed [3])
            assert(not released[3])

            local c = mouse.screen.clients[1]

            -- Make sure mixing the 2 syntaxes doesn't create a chimera state
            pressed[4], released[4] = false, false

            -- This *will* happen with older configs because `awful.rules` will
            -- loop the properties, find the old capi list and fail to understand
            -- that the content should use the legacy API.

            local joined = gtable.join(o1)

            assert(#joined == 4)

            c[type_name.."s"] = joined

            -- It should have been converted to the new format.
            assert(#c[type_name.."s"] == 1)

            return true
        end)

        table.insert(steps, function(count)
            if count < 5 then awesome.sync(); return end

            send_events(type_name, 3)
            send_events(type_name, 4)

            return true
        end)

        table.insert(steps, function()
            if (not pressed[3]) or (not released[3]) then return end

            assert(not pressed [4])
            assert(not released[4])

            local c = mouse.screen.clients[1]

            assert(#c[type_name.."s"] == 1)

            -- Test setting the object to `false` to simulate an inline Lua
            -- expression gone wrong. This used to work and is rather
            -- convenient, so lets not break it even if it is technically a bug.
            c[type_name.."s"] = false

            assert(#c[type_name.."s"] == 0)

            c[type_name.."s"] = {o1}

            assert(#c[type_name.."s"] == 1)

            -- Test removing the objects using `nil`.
            c[type_name.."s"] = nil

            assert(#c[type_name.."s"] == 0)

            c[type_name.."s"] = {o1}

            assert(#c[type_name.."s"] == 1)

            -- Test removing using `{}`
            c[type_name.."s"] = {}

            assert(#c[type_name.."s"] == 0)

            c:kill()

            return true
        end)

        -- Cleanup (otherwise there is a race with the root.buttons tests)
        table.insert(steps, function()
            if #mouse.screen.clients ~= 0 then return end

            return true
        end)
    end
end

local exe1, exe2, exe3, exe4 = false, false, false, false
local new1, new2 = nil, nil

-- Check that you can add new default key/mousebindings at any time.
table.insert(steps, function()
    assert(#mouse.screen.clients == 0)

    new1 = module.key {
        key       = "a",
        modifiers = {},
        on_press  = function() exe1 = true end
    }

    new2 = module.button {
        button    = 8,
        modifiers = {},
        on_press  = function() exe2 = true end
    }

    akeyboard.append_client_keybinding(new1)
    amouse.append_client_mousebinding(new2)

    -- Spawn a client.
    test_client("myclient")

    return true
end)

table.insert(steps, function()
    if #mouse.screen.clients == 0 then return end

    client.focus = mouse.screen.clients[1]

    -- That should trigger the newly added keybinding.
    root.fake_input("key_press"  , "a")
    root.fake_input("key_release", "a")

    -- Move the mouse over the client so mousebindings work.
    placement.centered(mouse, {parent=client.focus})
    awesome.sync()

    -- Same thing for the mouse.
    root.fake_input("button_press"  , 8)
    root.fake_input("button_release", 8)

    return true
end)

table.insert(steps, function()
    if (not exe1) or (not exe2) then return end

    akeyboard.remove_client_keybinding(new1)
    amouse.remove_client_mousebinding(new2)

    exe1, exe2 = false, false

    return true
end)

-- Removing is async, so wait until the next loop.
table.insert(steps, function()
    root.fake_input("key_press"  , "a")
    root.fake_input("key_release", "a")
    root.fake_input("button_press"  , 8)
    root.fake_input("button_release", 8)
    awesome.sync()

    return true
end)

-- Try adding key/mousebindings to existing clients.
table.insert(steps, function(count)
    if count ~= 3 then return end

    assert(not exe1)
    assert(not exe2)

    -- Append both types at the same time to make sure nothing overwrite
    -- the list.
    akeyboard.append_client_keybinding(new1)
    amouse.append_client_mousebinding(new2)

    new1 = module.key {
        key       = "b",
        modifiers = {},
        on_press  = function() exe3 = true end
    }

    new2 = module.button {
        button    = 7,
        modifiers = {},
        on_press  = function() exe4 = true end
    }

    -- Append directly on the client.
    client.focus:append_keybinding(new1)
    client.focus:append_mousebinding(new2)

    return true
end)

-- Removing is async, so wait until the next loop.
table.insert(steps, function()

    root.fake_input("key_press"  , "b")
    root.fake_input("key_release", "b")
    root.fake_input("button_press"  , 7)
    root.fake_input("button_release", 7)

    return true
end)

table.insert(steps, function()
    if (not exe3) or (not exe4) then return end

    -- Note, there is no point to try remove_keybinding, it was already called
    -- (indirectly) 4 steps ago.

    return true
end)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
