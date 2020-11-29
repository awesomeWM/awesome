--- Test for gears.watcher.

local runner = require("_runner")
local watcher = require("gears.watcher")
local gtimer = require("gears.timer")
local gdebug = require("gears.debug")
local filesystem = require("gears.filesystem")
local util = require("awful.util")

local obj1 = nil

local sig_count = {}

local steps = {
    function()
        obj1 = watcher {
            interval = 0.05,
            command  = "echo one two three | cut -f3 -d ' '",
            shell    = true,
            initial_value = 1337
        }

        assert(obj1.command == "echo one two three | cut -f3 -d ' '")
        assert(obj1.shell)
        assert(obj1.interval == 0.05)
        assert(obj1.value == 1337)

        return true
    end,
    -- Test the filters.
    function()
        if obj1.value ~= "three" then return end

        obj1.filter = function(lines)
            if lines == "three" then return "four" end
        end

        return true
    end,
    -- Switch to file mode.
    function()
        if obj1.value ~= "four" then return end

        obj1.filter = function(lines)
            return lines:match("theme") ~= nil
        end

        obj1:connect_signal("file::acquired", function()
            sig_count["file::acquired"] = sig_count["file::acquired"] or 0
            sig_count["file::acquired"] = sig_count["file::acquired"] + 1
        end)

        obj1:connect_signal("files::acquired", function()
            sig_count["files::acquired"] = sig_count["files::acquired"] or 0
            sig_count["files::acquired"] = sig_count["files::acquired"] + 1
        end)

        obj1:connect_signal("file::failed", function()
            sig_count["file::failed"] = sig_count["file::failed"] or 0
            sig_count["file::failed"] = sig_count["file::failed"] + 1
        end)

        obj1.file = filesystem.get_themes_dir().."/default/theme.lua"
        assert(obj1.file == filesystem.get_themes_dir().."/default/theme.lua")

        os.execute("echo 12345 > /tmp/test_gears_watcher1")
        os.execute("echo 67890 > /tmp/test_gears_watcher2")
        os.execute("rm -rf /tmp/test_gears_watcher3")

        return true
    end,
    -- Remove the filter.
    function()
        if obj1.value ~= true then return end

        assert(sig_count["file::acquired"] > 0)
        assert(sig_count["files::acquired"] > 0)
        assert(not sig_count["file::failed"])

        obj1.file = '/tmp/test_gears_watcher1'
        obj1.filter = nil

        return true
    end,
    -- Test strip_newline.
    function()
        if obj1.value ~= "12345" then return end

        obj1.strip_newline = false

        return true
    end,
    -- Test append_file.
    function()
        if obj1.value ~= "12345\n" then return end

        obj1.strip_newline = true
        obj1:append_file('/tmp/test_gears_watcher2', 'foo')

        return true
    end,
    -- Test remove_file.
    function()
        if type(obj1.value) ~= "table" or obj1.value["foo"] ~= "67890" then return end

        return true
    end,
    -- Test non-existant files.
    function()
        obj1.file = '/tmp/test_gears_watcher3'

        return true
    end,
    -- Test `started` and `:stop()`
    function()
        if not sig_count["file::failed"] then return end

        assert(obj1.started)
        obj1.started = false
        assert(not obj1.started)

        local real_error = gdebug.print_error

        local called = false

        gdebug.print_error = function()
            called = true
        end

        obj1:stop()
        assert(not obj1.started)
        assert(called)
        called = false

        obj1:start()
        assert(obj1.started)
        assert(not called)
        obj1:stop()
        assert(not obj1.started)
        assert(not called)

        gdebug.print_error = real_error

        return true
    end,
    -- Test `awful.util.shell` and `gears.timer` compatibility mode.
    function()
        assert(util.shell == watcher._shell)
        watcher._shell = "foo"
        assert(util.shell == "foo")
        util.shell = "bar"
        assert(watcher._shell == "bar")

        local called = false

        local real_deprecate = gdebug.deprecate
        gdebug.deprecate = function()
            called = true
        end

        local t = gtimer {
            timeout = 5
        }

        assert(t.data.timeout)
        assert(called)
        called = false
        t.data.foo = "bar"
        assert(t._private.foo == "bar")
        assert(called)
        t._private.foo = "baz"
        assert(t.data.foo == "baz")

        gdebug.deprecate = real_deprecate

        return true
    end,
}
runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
