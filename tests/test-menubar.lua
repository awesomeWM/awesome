-- Test the menubar

local runner = require("_runner")
local menubar = require("menubar")
local gears_debug = require("gears.debug")

local assets_dir = os.getenv("TEST_ASSETS") .. "/"
menubar.menu_gen.all_menu_dirs = { assets_dir }
local capi = {
    screen = screen
}

local menubar_refreshed = false
local orig_refresh = menubar.refresh
function menubar.refresh(...)
    menubar_refreshed = true
    orig_refresh(...)
end

-- XXX We are building Lua 5.3 with -DLUA_USE_APICHECK=1 and this catches some
-- bugs in lgi. Thus, do not run this test with Lua 5.3 until lgi is fixed.
-- We run into the same bug when doing code-coverage analysis, supposedly
-- because the additional GC activity means that something which is GC-able when
-- it should not gets collected too early.
-- This test also sporadically errors on LuaJIT ("attempt to call a number
-- value"). This might be a different issue, but still, disable the test.
-- 
-- 
--[[
if _VERSION == "Lua 5.3" or debug.gethook() or jit then --luacheck: globals jit
print("Skipping this test since it would just fail.")
runner.run_steps { function() return true end }
return
end
--]]

-- Make sure no error messages for non-existing directories are printed. If the
-- word "error" appears in our output, the test is considered to have failed.
do
    local gdebug = require("gears.debug")
    local orig_error = gdebug.print_error
    function gdebug.print_error(msg)
        msg = tostring(msg)
        if (msg ~= "Error opening directory '/dev/null/.local/share/applications': Not a directory") and
            (not msg:match("No such file or directory")) then
            orig_error(msg)
        end
    end
end

runner.run_steps {
    function()
        -- Make sure that the menubar doesn't have any windows before we create it
        local mbar_guts = menubar.__testing_internals()
        assert(mbar_guts.instance == nil)
        return true
    end,
    function(count)
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
    end,

    -- Common.list_update can be tested elsewhere

    -- Test internal `get_current_page` function
    -- Not sure how to do this, tackle later
    function()
        return true
    end,

    -- Test common args wibox
    function()
        local mbar_guts = menubar.__testing_internals()
        -- this is menubar.common_args.w
        -- which is only set by `commmon.list_update`
        local w = mbar_guts.common_args.w
        assert(w._private.dir == "x")
        -- It was 11 the last time I checked it.
        -- im coder
        assert(#w:get_children() == 11)
        return true
    end,

    -- Test internal `get_screen` function
    -- tbh this is kind of silly and only tests for "primary" and nil - oh well
    function()
        local mbar_guts = menubar.__testing_internals()
        local s = mbar_guts.get_screen("primary")
        assert(s == capi.screen.primary)
        s = mbar_guts.get_screen(nil)
        assert(s == nil)
        return true
    end,

    -- Test `write_count_table` function
    function()
        -- Not sure what to do
        return true
    end,

    -- Test `load_count_table` function
    -- Used twice - in `perform_action` and in `menulist_update`
    function()
        local mbar_guts = menubar.__testing_internals()
        local count_table = mbar_guts.load_count_table()
        assert(#count_table == 0) -- ??? It was empty upon a debug dump
        return true
    end,

    -- Test `perform_action` function
    -- Used only once in `prompt_keypressed_callback`
    function()
        local mbar_guts = menubar.__testing_internals()
        local a, newcmd, newprompt = mbar_guts.perform_action({
            key = "Testing",
            cmdline = "ls",
            name = "example"
        })
        assert(a == true)
        assert(newcmd == "")
        assert(newprompt == "example: ")
        a, newcmd, newprompt = mbar_guts.perform_action({
            cmdline = ":", -- Should be a no-op command
            name = "example2"
        })
        assert(mbar_guts.instance)
        assert(mbar_guts.instance.count_table)
        assert(mbar_guts.instance.count_table["example2"] == 1)
        return true
    end,

    -- Test `label` function
    function()
        local mbar_guts = menubar.__testing_internals()
        local tname, tcolor, tnil, ticon = mbar_guts.label(
        {
            name = "foobaz",
            focused = false,
            icon = "1234.png"
        })
        assert(tname == "foobaz")
        assert(tcolor == "#222222")
        assert(tnil == nil)
        assert(ticon == "1234.png")
        tname, tcolor, tnil, ticon = mbar_guts.label(
        {
            name = "foobar",
            focused = true,
            icon = "4321.png"
        })
        assert(tname == "<span color='#ffffff'>foobar</span>")
        assert(tcolor == "#535d6c")
        assert(tnil == nil)
        assert(ticon == "4321.png")
        return true
    end, 

    -- Test `menulist_update` function
    function()
        return true
    end,

    function()
        return true
    end
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
