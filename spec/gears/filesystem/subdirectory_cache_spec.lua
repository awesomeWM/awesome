---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

-- Fake news?
local fake_time = os.time()
_G.os.time = function()
    return fake_time
end

local dir_cache = require("gears.filesystem.subdirectory_cache")
local protected_call = require("gears.protected_call")
local gio = require("lgi").Gio

describe("gears.filesystem.subdirectory_cache", function()
    describe("finds files", function()
        -- This assumes that we are run from awesome's source directory
        gio.Async.call(protected_call)(function()
            local cache = dir_cache.async_new("spec/gears")
            assert.is_true(cache:async_check_exists("filesystem/subdirectory_cache_spec.lua"))
            assert.is_true(cache:async_check_exists("cache_spec.lua"))
            assert.is_false(cache:async_check_exists(""))
            assert.is_false(cache:async_check_exists("something weird"))
        end)
    end)

    describe("non-existing directory", function()
        gio.Async.call(protected_call)(function()
            local gdebug = require("gears.debug")
            local print_warning = gdebug.print_warning
            local called
            function gdebug.print_warning(message)
                assert(message:find("/this/does not/exist/please"), message)
                called = true
            end

            local cache = dir_cache.async_new("/this/does not/exist/please")
            assert(called)
            gdebug.print_warning = print_warning

            assert.is_false(cache:async_check_exists(""))
            assert.is_false(cache:async_check_exists("something weird"))
            assert.is.same({}, cache.known_paths)
        end)
    end)

    describe("cache expiry", function()
        local test_path = gio.File.new_tmp("awesome-tests-path-XXXXXX")
        local dir = test_path:get_child("dir")
        local file1 = test_path:get_child("file1")
        local file2 = dir:get_child("file2")

        test_path:delete()
        assert(test_path:make_directory())

        assert(gio.Async.call(protected_call)(function()
            -- At this point the directory is empty
            local cache = dir_cache.async_new(test_path:get_path())
            assert.is.same({}, cache.known_paths)

            -- Create some files
            assert(dir:make_directory())
            assert(file1:create(gio.FileCreateFlags.NONE):async_close())
            assert(file2:create(gio.FileCreateFlags.NONE):async_close())

            -- The cache still doesn't know about these files
            cache:async_update()
            assert.is.same({}, cache.known_paths)

            -- Let the cache think some time has passed so that it updates.
            fake_time = fake_time + 6
            cache:async_update()
            assert.is.same({}, cache.known_paths)

            -- Plus, mess with the mtime of the file.
            fake_time = fake_time + 6
            assert(test_path:set_attribute_uint64("time::modified",
                    fake_time, gio.FileQueryInfoFlags.NONE))
            cache:async_update()

            assert(cache.known_paths[file1:get_path()])
            assert(cache.known_paths[file2:get_path()])

            local count = 0
            for _ in pairs(cache.known_paths) do
                count = count + 1
            end
            assert.is.equal(count, 2)

            return true
        end))

        file1:delete()
        file2:delete()
        dir:delete()
        test_path:delete()
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
