-- Test the menubar

local runner = require("_runner")
local menubar = require("menubar")

-- XXX We are building Lua 5.3 with -DLUA_USE_APICHECK=1 and this catches some
-- bugs in lgi. Thus, do not run this test with Lua 5.3 until lgi is fixed.
-- We run into the same bug when doing code-coverage analysis, supposedly
-- because the additional GC activity means that something which is GC-able when
-- it should not gets collected too early
if _VERSION == "Lua 5.3" or debug.gethook() then
    print("Skipping this test since it would just fail.")
    runner.run_steps { function() return true end }
    return
end

runner.run_steps {
    function(count)
        -- Just show the menubar and hide it.
        -- TODO: Write a proper test. But for the mean time this is better than
        -- nothing (and tells us when errors are thrown).

        if count == 1 then
            -- Remove the entry for $HOME/.... from the search path. We run with
            -- HOME=/dev/null and this would cause an error message to be
            -- printed which would be interpreted as the test failing
            local entry = menubar.menu_gen.all_menu_dirs[1]
            assert(string.find(entry, "/dev/null"), entry)
            table.remove(menubar.menu_gen.all_menu_dirs, 1)

            menubar.show()
        end

        -- Test that the async population of the menubar is done
        if #menubar.menu_entries > 0 then
            menubar.hide()
            awesome.sync()
            return true
        end
    end,

    function()
        return true
    end
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
