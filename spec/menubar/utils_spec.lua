---------------------------------------------------------------------------
-- @author Zach Peltzer
-- @copyright 2017 Zach Peltzer
---------------------------------------------------------------------------
-- just for educational testing
local utils = require("menubar.utils")
local theme = require("beautiful")
--local glib = require("lgi").GLib

describe("menubar.utils lookup_icon_uncached", function()
    local shimmed = {}
    local icon_theme

    local function assert_found_in_path(icon, path)
        assert.matches(path .. '$', utils.lookup_icon_uncached(icon) or '')
    end

    setup(function()
        local root = (os.getenv("SOURCE_DIRECTORY") or '.') .. "/spec/menubar"

        local function shim(name, retval)
            shimmed[name] = name --glib[name]
--            glib[name] = function() return retval end
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
--            glib[name] = func
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
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
