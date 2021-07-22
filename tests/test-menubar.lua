-- Test the menubar

local runner = require("_runner")
local menubar = require("menubar")

local menubar_refreshed = false
local orig_refresh = menubar.refresh
function menubar.refresh(...)
    menubar_refreshed = true
    orig_refresh(...)
end

-- XXX This test sporadically errors on LuaJIT ("attempt to call a number
-- value"). This might be a different issue, but still, disable the test.
-- And also breaks other tests due to weirdness of older lgi version.
if os.getenv('LGIVER') == '0.8.0' or jit then --luacheck: globals jit
    print("Skipping this test since it would just fail.")
    runner.run_steps { function() return true end }
    return
end

-- Make sure no error messages for non-existing directories are printed. If the
-- word "error" appears in our output, the test is considered to have failed.
do
    local gdebug = require("gears.debug")
    local orig_warning = gdebug.print_warning
    function gdebug.print_warning(msg)
        msg = tostring(msg)
        if (not msg:match("/dev/null/.local/share/applications")) and
                (not msg:match("No such file or directory")) then
            orig_warning(msg)
        end
    end
end

local show_menubar_and_hide = function(count)
    -- Just show the menubar and hide it.
    -- TODO: Write a proper test. But for the mean time this is better than
    -- nothing (and tells us when errors are thrown).

    if count == 1 then
        menubar.show()
    end

    -- Test that the async population of the menubar is done
    if menubar_refreshed then
        menubar.hide()
        awesome.sync()
        return true
    end
end

runner.run_steps {
    function(count)
        -- Show and hide with defaults
        return show_menubar_and_hide(count)
    end,

    function(count)
        -- Show and hide with match_empty set to false
        menubar.match_empty = false
        return show_menubar_and_hide(count)
    end,

    function()
        return true
    end
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
