--- Tests for urgent property.

local awful = require("awful")
local runner = require("_runner")

-- Some basic assertion that the tag is not marked "urgent" already.
assert(awful.tag.getproperty(awful.screen.focused().tags[2], "urgent") == nil)


-- Setup signal handler which should be called.
-- TODO: generalize and move to runner.
local urgent_cb_done
client.connect_signal("property::urgent", function (c)
    urgent_cb_done = true
    assert(c.class == "XTerm", "Client should be xterm!")
end)

local manage_cb_done
client.connect_signal("request::manage", function (c)
    manage_cb_done = true
    assert(c.class == "XTerm", "Client should be xterm!")
end)


-- Steps to do for this test.
local steps = {
    -- Step 1: tag 2 should become urgent, when a client gets tagged via rules.
    function(count)
        if count == 1 then  -- Setup.
            urgent_cb_done = false
            -- Select first tag.
            awful.screen.focused().tags[1]:view_only()

            runner.add_to_default_rules({ rule = { class = "XTerm" },
                                        properties = { tag = "2", focus = true } })

            awful.spawn("xterm")
        end
        if urgent_cb_done then
            assert(awful.tag.getproperty(awful.screen.focused().tags[2], "urgent") == true)
            assert(awful.tag.getproperty(awful.screen.focused().tags[2], "urgent_count") == 1)
            return true
        end
    end,

    -- Step 2: when switching to tag 2, it should not be urgent anymore.
    function(count)
        if count == 1 then
            -- Setup: switch to tag.
            root.fake_input("key_press", "Super_L")
            root.fake_input("key_press", "2")
            root.fake_input("key_release", "2")
            root.fake_input("key_release", "Super_L")

        elseif awful.screen.focused().selected_tags[1] == awful.screen.focused().tags[2] then
            assert(#client.get() == 1)
            local c = client.get()[1]
            assert(not c.urgent, "Client is not urgent anymore.")
            assert(c.active, "Client is focused.")
            assert(awful.tag.getproperty(awful.screen.focused().tags[2], "urgent") == false)
            assert(awful.tag.getproperty(awful.screen.focused().tags[2], "urgent_count") == 0)
            return true
        end
    end,

    -- Step 3: tag 2 should not be urgent, but switched to.
    function(count)
        if count == 1 then  -- Setup.
            urgent_cb_done = false

            -- Select first tag.
            awful.screen.focused().tags[1]:view_only()

            runner.add_to_default_rules({ rule = { class = "XTerm" },
                                        properties = { tag = "2", focus = true, switch_to_tags = true }})

            awful.spawn("xterm")

        elseif awful.screen.focused().selected_tags[1] == awful.screen.focused().tags[2] then
            assert(not urgent_cb_done)
            assert(awful.tag.getproperty(awful.screen.focused().tags[2], "urgent") == false)
            assert(awful.tag.getproperty(awful.screen.focused().tags[2], "urgent_count") == 0)
            assert(awful.screen.focused().selected_tags[2] == nil)
            return true
        end
    end,


    -- Step 4: tag 2 should not become urgent, when a client gets tagged via
    -- rules with focus=false.
    function(count)
        if count == 1 then  -- Setup.
            client.get()[1]:kill()
            manage_cb_done = false

            runner.add_to_default_rules({rule = { class = "XTerm" },
                                        properties = { tag = "2", focus = false }})

            awful.spawn("xterm")
        end
        if manage_cb_done then
            assert(client.get()[1].urgent == false)
            assert(awful.tag.getproperty(awful.screen.focused().tags[2], "urgent") == false)
            assert(awful.tag.getproperty(awful.screen.focused().tags[2], "urgent_count") == 0)
            return true
        end
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
