---------------------------------------------------------------------------
-- @author Zach Peltzer
-- @copyright 2017 Zach Peltzer
---------------------------------------------------------------------------

local utils = require("menubar.utils")
local theme = require("beautiful")
local glib = require("lgi").GLib
local gfs = require("gears.filesystem")

describe("menubar.utils lookup_icon_uncached", function()
    local shimmed = {}
    local gfs_shim_dir_readable
    local gfs_shim_file_readable
    local icon_theme

    local function assert_found_in_path(icon, path)
        assert.matches(path .. '$', utils.lookup_icon_uncached(icon) or '')
    end

    setup(function()
        local root = (os.getenv("SOURCE_DIRECTORY") or '.') .. "/spec/menubar"

        local function shim(name, retval)
            shimmed[name] = glib[name]
            glib[name] = function() return retval end
        end

        shim('get_home_dir',         "/home")
        shim('get_user_data_dir',    "/home/.local/share")
        shim('get_system_data_dirs', {
            "/usr/local/share",
            "/usr/share"
        })

        gfs_shim_dir_readable = gfs.dir_readable
        gfs.dir_readable = function(path) return gfs_shim_dir_readable(root..path) end
        gfs_shim_file_readable = gfs.file_readable
        gfs.file_readable = function(filename) return gfs_shim_file_readable(root..filename) end

        icon_theme = theme.icon_theme
        theme.icon_theme = 'awesome'
    end)

    teardown(function()
        for name, func in pairs(shimmed) do
            glib[name] = func
        end
        gfs.dir_readable = gfs_shim_dir_readable
        gfs.file_readable = gfs_shim_file_readable
        theme.icon_theme = icon_theme
    end)

    it('finds icons in icon base directories, in correct order', function()

        -- Shimmed icon base directories contain the following icons:
        --
        --     usr/share/pixmaps/icon[1-5].png
        --     usr/share/icons/icon[2-5].png
        --     usr/local/share/icons/icon[3-5].png
        --     .local/share/icons/icon[4-5].png
        --     .icons/icon5.png

        assert_found_in_path('icon1', '/usr/share/pixmaps/icon1.png')
        assert_found_in_path('icon2', '/usr/share/icons/icon2.png')
        assert_found_in_path('icon3', '/usr/local/share/icons/icon3.png')
        assert_found_in_path('icon4', '/.local/share/icons/icon4.png')
        assert_found_in_path('icon5', '/.icons/icon5.png')
    end)

    it('finds icons in $HOME/.icons/<theme>/<size>/apps/', function()

        -- Theme 'awesome' in shimmed $HOME/.icons:

        assert_found_in_path('awesome',  '/.icons/awesome/64x64/apps/awesome.png')
        assert_found_in_path('awesome2', '/.icons/awesome/scalable/apps/awesome2.png')
    end)

    -- No argument
    it('no icon specified', function()
        assert.is_false(utils.lookup_icon_uncached())
        assert.is_false(utils.lookup_icon_uncached(nil))
        assert.is_false(utils.lookup_icon_uncached(false))
        assert.is_false(utils.lookup_icon_uncached(''))
    end)

    -- Full path and filename with expected extension
    it('finds icons even those not in the search paths when full path is specified', function()

        -- Shimmed icon base directories contain the following icons:
        --
        --     usr/share/icon5.png
        --     usr/share/icon6.xpm
        --     usr/share/icon7.svg
        --     usr/share/icons/.filename.png

        assert_found_in_path('/usr/share/icon5.png', '/usr/share/icon5.png')
        assert_found_in_path('/usr/share/icon6.xpm', '/usr/share/icon6.xpm')
        assert_found_in_path('/usr/share/icon7.svg', '/usr/share/icon7.svg')

        assert_found_in_path('/usr/share/.filename.png', '/usr/share/.filename.png')

        assert.is_nil(utils.lookup_icon_uncached('/blah/icon6.png')) -- supported file does not exist in location
        assert.is_nil(utils.lookup_icon_uncached('/.png')) -- path & supported ext - but also not found
    end)

   -- Filename with supported extension
    it('finds icons with specified supported extension in the search path', function()

        -- Shimmed icon base directories contain the following icons:
        --
        --     .icons/icon5.png
        --     .icons/icon6.xpm
        --     .icons/icon7.svg
        --     usr/share/icons/.filename.png

        assert_found_in_path('icon5.png', '/.icons/icon5.png')
        assert_found_in_path('icon6.xpm', '/.icons/icon6.xpm')
        assert_found_in_path('icon7.svg', '/.icons/icon7.svg')

        assert_found_in_path('.filename.png', '/usr/share/icons/.filename.png')

        --  Will fail as file does not exist (but extension is supported)
        assert.is_false(utils.lookup_icon_uncached('icon8.png'))
    end)

    -- Find icon with no (or invalid) extension matching
    it('finds icons without specified path or extension', function()

        -- Shimmed icon base directories contain the following icons:
        --
        --     .icons/icon6.xpm
        --     .icons/icon7.svg
        --     usr/share/icons/.filename.png

        assert_found_in_path('icon6', '/.icons/icon6.xpm')
        assert_found_in_path('icon7', '/.icons/icon7.svg')

        assert_found_in_path('.filename', '/usr/share/icons/.filename.png')

        assert.is_false(utils.lookup_icon_uncached('/png')) -- path & no ext
        assert.is_false(utils.lookup_icon_uncached('.png')) -- file does not exist
        assert.is_false(utils.lookup_icon_uncached('png')) -- file does not exist
        assert.is_false(utils.lookup_icon_uncached('icon9')) -- file does not exist
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
