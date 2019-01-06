---------------------------------------------------------------------------
-- @author Sorky
-- @copyright 2019 Sorky
---------------------------------------------------------------------------

local gfs = require("gears.filesystem")
local Gio = {File               = require("lgi").Gio.File,
             FileInfo           = require("lgi").Gio.FileInfo,
             FileQueryInfoFlags = require("lgi").Gio.FileQueryInfoFlags}

describe("gears.filesystem file_readable", function()
    local shimmed_File = {}
    local shimmed_FileInfo = {}
    local shimmed_FileQueryInfoFlags = {}
    local query_info

    setup(function()
        local function shim_File(name, retval)
            shimmed_File[name] = Gio.File[name]
            Gio.File[name] = function() return retval end
        end
        local function shim_FileInfo(name, retval)
            shimmed_FileInfo[name] = Gio.FileInfo[name]
            Gio.FileInfo[name] = function() return retval end
        end
        local function shim_FileQueryInfoFlags(name, retval)
            shimmed_FileQueryInfoFlags[name] = Gio.FileQueryInfoFlags[name]
            Gio.FileQueryInfoFlags[name] = function() return retval end
        end

        shim_FileQueryInfoFlags('NONE', '0.0')
        shim_File('new_for_path', 'lgi.obj 0x557958892160:GObject.Object(GLocalFile)')
        shim_File('query_info', 'lgi.obj 0x55795ba08870:Gio.FileInfo(GFileInfo)')
        shim_FileInfo('get_file_type', 'REGULAR')
        shim_FileInfo('get_attribute_boolean', 'true')

        query_info = Gio.File.query_info
    end)

    teardown(function()
        for name, func in pairs(shimmed) do
            Gio[name] = func
        end
        for name, func in pairs(shimmed_File) do
            Gio.File[name] = func
        end
        for name, func in pairs(shimmed_FileInfo) do
            Gio.FileInfo[name] = func
        end
        for name, func in pairs(shimmed_FileQueryInfoFlags) do
            Gio.FileQueryInfoFlags[name] = func
        end
    end)

    it('test', function()
        assert.matches('', gfs.file_readable('/icons/awesome16.png'))
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80function filesystem.file_readable(filename)
