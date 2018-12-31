require("awful._compat")

local runner      = require( "_runner"         )
local placement   = require( "awful.placement" )
local gtable      = require( "gears.table"     )
local test_client = require( "_client"         )

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

            o1 = module[type_name](
                {"Mod1"}, second[type_name][3], function() pressed[3] = true end, function() released[3] = true end
            )

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
            c[type_name.."s"] = gtable.join(o1)

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

            assert(#c[type_name.."s"] ~= 0)

            -- Test setting the object to `false` to simulate an inline Lua
            -- expression gone wrong. This used to work and is rather
            -- convenient, so lets not break it even if it is technically a bug.
            c[type_name.."s"] = false

            assert(#c[type_name.."s"] == 0)

            c[type_name.."s"] = {o1}

            assert(#c[type_name.."s"] ~= 0)

            -- Test removing the objects using `nil`.
            c[type_name.."s"] = nil

            assert(#c[type_name.."s"] == 0)

            c[type_name.."s"] = {o1}

            assert(#c[type_name.."s"] ~= 0)

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

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
