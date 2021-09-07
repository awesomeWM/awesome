---------------------------------------------------------------------------
-- @author
-- @copyright 2019
---------------------------------------------------------------------------

local gfs = require("gears.filesystem")

describe("gears.filesystem", function()
    local root = (os.getenv("SOURCE_DIRECTORY") or '.') .. "/spec/gears/"

    -- Check filesystem.make_directories
    it('Check filesystem.make_directories', function()
        assert.is_true(gfs.make_directories(root .. "filesystem_tests/x/y"))
    end)

    -- Check filesystem.make_parent_directories
    it('Check filesystem.make_parent_directories', function()
        assert.is_true(gfs.make_directories(root .. "filesystem_tests/x/y"))
    end)

    -- Check filesystem.file_readable
    it('Check filesystem.file_readable', function()
        os.execute("chmod g-r,o-r,a-r " .. root .. "filesystem_tests/x/NoRead")

        assert.is_true(gfs.file_readable(root .. "filesystem_tests/x/Read"))
        assert.is_false(gfs.file_readable(root .. "filesystem_tests/x/NoRead"))
        assert.is_nil(gfs.file_readable(root .. "filesystem_tests/x/NO_FILE"))

        os.execute("chmod g+r,o+r,a+r " .. root .. "filesystem_tests/x/NoRead")
    end)

    -- Check filesystem.file_executable
    it('Check filesystem.file_executable', function()
        assert.is_true(gfs.file_executable(root .. "filesystem_tests/x/Exec"))
        assert.is_false(gfs.file_executable(root .. "filesystem_tests/x/NoExec"))
        assert.is_nil(gfs.file_executable(root .. "filesystem_tests/x/NO_FILE"))
    end)

    -- Check filesystem.dir_readable
    it('Check filesystem.dir_readable', function()
        os.execute("chmod g-r,o-r,a-r " .. root .. "filesystem_tests/y")

        assert.is_true(gfs.dir_readable(root .. "filesystem_tests/x"))
        assert.is_false(gfs.dir_readable(root .. "filesystem_tests/y"))
        assert.is_nil(gfs.dir_readable(root .. "filesystem_tests/NO_DIR"))

        os.execute("chmod g+r,o+r,a+r " .. root .. "filesystem_tests/y")
    end)

    -- Check filesystem.is_dir
    it('Check filesystem.is_dir', function()
        assert.is_true(gfs.is_dir(root .. "filesystem_tests/x/y"))
        assert.is_false(gfs.is_dir(root .. "filesystem_tests/x/Read"))
        -- Following would be nicer if it returned nil but it is what it is
        assert.is_false(gfs.is_dir(root .. "filesystem_tests/x/NO_DIR"))
    end)

    -- Check filesystem.get_xdg_config_home
    it('Check filesystem.get_xdg_config_home', function()
        -- TODO: assert.is_same("?", gfs.get_xdg_config_home())
    end)

    -- Check filesystem.get_xdg_cache_home
    it('Check filesystem.get_xdg_cache_home', function()
        -- TODO: assert.is_same("?", gfs.get_xdg_cache_home())
    end)

    -- Check filesystem.get_xdg_data_home
    it('Check filesystem.get_xdg_data_home', function()
        -- TODO: assert.is_same("?", gfs.get_xdg_data_home())
    end)

    -- Check filesystem.get_xdg_data_dirs
    it('Check filesystem.get_xdg_data_dirs', function()
        -- TODO: assert.is_same("?", gfs.get_xdg_data_dirs())
    end)

    -- Check filesystem.get_configuration_dir
    it('Check filesystem.get_configuration_dir', function()
        -- TODO: assert.is_same("?", gfs.get_configuration_dir())
    end)

    -- Check filesystem.get_cache_dir
    it('Check filesystem.get_cache_dir', function()
        -- TODO: assert.is_same("?", gfs.get_cache_dir())
    end)

    -- Check filesystem.get_themes_dir
    it('Check filesystem.get_themes_dir', function()
        -- TODO: assert.is_same("?", gfs.get_themes_dir())
    end)

    -- Check filesystem.get_awesome_icon_dir
    it('Check filesystem.get_awesome_icon_dir', function()
        -- TODO: assert.is_same("?", gfs.get_awesome_icon_dir())
    end)

    -- Check filesystem.get_random_file_from_dir
    it('Check filesystem.get_random_file_from_dir', function()
        -- No file
        assert.is_nil(gfs.get_random_file_from_dir(root .. "filesystem_tests", {"bmp"}))

        -- File found matching extension
        assert.is_same("a.png", gfs.get_random_file_from_dir(root .. "filesystem_tests", {"png"}))
        assert.is_same("b.jpg", gfs.get_random_file_from_dir(root .. "filesystem_tests", {"jpg"}))

        for _ = 1, 20 do
            -- Any file found (any extension)
            local test_a = gfs.get_random_file_from_dir(root .. "filesystem_tests")
            assert.is_true(test_a == "a.png"
                        or test_a == "b.jpg")

            -- Any file found (selected extensions)
            local test_b = gfs.get_random_file_from_dir(root .. "filesystem_tests", {"png", "jpg"})
            assert.is_true(test_b == "a.png"
                        or test_b == "b.jpg")

            -- Any file found (selected extensions)
            local test_c = gfs.get_random_file_from_dir(root .. "filesystem_tests", {".png", ".jpg"})
            assert.is_true(test_c == "a.png"
                        or test_c == "b.jpg")

            -- Test absolute paths.
            local test_d = gfs.get_random_file_from_dir(root .. "filesystem_tests", {".png", ".jpg"}, true)
            assert.is_true(test_d == root .. "filesystem_tests/a.png"
                        or test_d == root .. "filesystem_tests/b.jpg")

            -- Make sure the paths are generated correctly.
            assert.is_nil(test_d:match("//"))

            -- "." in filename test cases with extensions
            local test_e = gfs.get_random_file_from_dir(root .. "filesystem_tests/y", {"ext"})
            assert.is_true(test_e == "filename.ext"
                         or test_e == ".filename.ext"
                        or test_e == "file.name.ext"
                        or test_e == ".file.name.ext")

            -- "." in filename test cases with no extensions
            local test_f = gfs.get_random_file_from_dir(root .. "filesystem_tests/y", {""})
            assert.is_true(test_f == "filename"
                        or test_f == "filename."
                        or test_f == "filename.ext."
                        or test_f == ".filename"
                        or test_f == ".filename."
                        or test_f == "file.name.ext."
                        or test_f == ".file.name.ext.")

            -- Test invalid directories.
            local test_g = gfs.get_random_file_from_dir(root .. "filesystem_tests/fake_dir")
            assert.is_nil(test_g)

        end
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
