-- Test the menubar

local runner = require("_runner")
local menubar = require("menubar")

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
if _VERSION == "Lua 5.3" or debug.gethook() or jit then --luacheck: globals jit
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

runner.run_steps {
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

    -- Test menubar.utils.unescape()
    function()
        -- Single strings
        local single_strings = {
            [ [[\n:\r:\s:\t:\\]] ] = "\n:\r: :\t:\\",
            -- Make sure escapes are not read recursively
            [ [[\\s]] ] = [[\s]],
            -- Make sure ';' is not escaped for non-list strings and other
            -- characters are not escaped
            [ [[ab\c\;1\23]] ] = [[ab\c\;1\23]],
        }

        for escaped, unescaped in pairs(single_strings) do
            if menubar.utils.unescape(escaped, false) ~= unescaped then
                return false
            end
        end

        -- Semicolon-separated lists of strings
        local list_strings = {
            -- Normal list
            [ [[abc;123;xyz]] ] = { "abc", "123", "xyz" },
            -- Optional terminating semicolon
            [ [[abc;123;xyz;]] ] = { "abc", "123", "xyz" },
            -- Blank item
            [ [[abc;;123]] ] = { "abc", "", "123" },
            -- Escape semicolon
            [ [[abc\;;12\;3;\;xyz]] ] = { "abc;", "12;3", ";xyz" },
            -- Normal escapes are parsed like normal
            [ [[ab\c;1\s23;x\\yz]] ] = { "ab\\c", "1 23", "x\\yz" },
            -- Escaped backslashes before semicolon
            [ [[abc;12\\;3;xyz]] ] = { "abc", "12\\", "3", "xyz" },
        }

        for escaped, unescaped in pairs(list_strings) do
            local returned = menubar.utils.unescape(escaped, true)
            if #returned ~= #unescaped then
                return false
            end

            for i = 1, #returned do
                if returned[i] ~= unescaped[i] then
                    return false
                end
            end
        end

        return true
    end,

    function()
        return true
    end
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
