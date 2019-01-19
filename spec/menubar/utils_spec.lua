---------------------------------------------------------------------------
-- @author Zach Peltzer
-- @copyright 2017 Zach Peltzer
---------------------------------------------------------------------------

local utils = require("menubar.utils")
local theme = require("beautiful")
local glib = require("lgi").GLib

describe("menubar.utils lookup_icon_uncached", function()
    local shimmed = {}
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

        shim('get_home_dir',         root .. "/home")
        shim('get_user_data_dir',    root .. "/home/.local/share")
        shim('get_system_data_dirs', {
            root .. "/usr/local/share",
            root .. "/usr/share"
        })

        icon_theme = theme.icon_theme
        theme.icon_theme = 'awesome'
    end)

    teardown(function()
        for name, func in pairs(shimmed) do
            glib[name] = func
        end
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

-- Check for no argument :: if not icon_file or icon_file == "" then
    it('no icon specified', function()
        assert.is_false(utils.lookup_icon_uncached())
        assert.is_false(utils.lookup_icon_uncached(nil)) -- Same as above?
        assert.is_false(utils.lookup_icon_uncached(false))
        assert.is_false(utils.lookup_icon_uncached(''))
    end)

-- Check for icon not in search path :: if icon_file:sub(1, 1) == '/' and supported_icon_formats[icon_file_ext] then
    it('finds icons even those not in the search paths when full path specified', function()

        -- Shimmed icon base directories contain the following icons:
        --
        --     usr/share/icon5.png
        --     usr/share/icon6.xpm
        --     usr/share/icon7.svg

        assert_found_in_path('/spec/menubar/usr/share/icon5.png', '/spec/menubar/usr/share/icon5.png')
        assert_found_in_path('/spec/menubar/usr/share/icon6.xpm', '/spec/menubar/usr/share/icon6.xpm')
        assert_found_in_path('/spec/menubar/usr/share/icon7.svg', '/spec/menubar/usr/share/icon7.svg')

        assert.is_same(nil, utils.lookup_icon_uncached('/.png')) -- supported file does not exist in location
        assert.is_same(nil, utils.lookup_icon_uncached('/blah/icon6.png')) -- supported file does not exist in location
    end)

-- Check icon with specified extension matching :: if supported_icon_formats[icon_file_ext] and
    it('finds icons with specified supported extension in search path', function()

        -- Shimmed icon base directories contain the following icons:
        --
        --     .icons/icon5.png
        --     .icons/icon6.xpm
        --     .icons/icon7.svg

        assert_found_in_path('icon5.png', '/.icons/icon5.png')
        assert_found_in_path('icon6.xpm', '/spec/menubar/.icons/icon6.xpm')
        assert_found_in_path('icon7.svg', '/spec/menubar/.icons/icon7.svg')

        --  Will fail as file does not exist (but extension is supported)
        assert.is_false(utils.lookup_icon_uncached('icon8.png'))
    end)

-- Check icon with no (or invalid) extension matching :: if NOT supported_icon_formats[icon_file_ext]
    it('finds icons without specified path or extension', function()

        -- Shimmed icon base directories contain the following icons:
        --
        --     .icons/icon6.xpm
        --     .icons/icon7.svg

        assert_found_in_path('icon6', '/spec/menubar/.icons/icon6.xpm') -- Similar to original tests and testing xpm extension
        assert_found_in_path('icon7', '/spec/menubar/.icons/icon7.svg') -- Similar to original tests and testing svg extension

        assert.is_false(utils.lookup_icon_uncached('/png')) -- supported file does not exist in given location
        assert.is_false(utils.lookup_icon_uncached('.png')) -- file does not exist
        assert.is_false(utils.lookup_icon_uncached('png')) -- file does not exist
        assert.is_false(utils.lookup_icon_uncached('icon9')) -- file does not exist
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
