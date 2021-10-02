---------------------------------------------------------------------------
--- Filesystem module for gears.
--
-- @utillib gears.filesystem
---------------------------------------------------------------------------

-- Grab environment we need
local Gio = require("lgi").Gio
local gstring = require("gears.string")
local gtable = require("gears.table")

local filesystem = {}

local function make_directory(gfile)
    local success, err = gfile:make_directory_with_parents()
    if success then
        return true
    end
    if err.domain == Gio.IOErrorEnum and err.code == "EXISTS" then
        -- Directory already exists, let this count as success
        return true
    end
    return false, err
end

--- Create a directory, including all missing parent directories.
-- @tparam string dir The directory.
-- @return (true, nil) on success, (false, err) on failure
-- @staticfct gears.filesystem.make_directories
function filesystem.make_directories(dir)
    return make_directory(Gio.File.new_for_path(dir))
end

function filesystem.mkdir(dir)
    require("gears.debug").deprecate("gears.filesystem.make_directories", {deprecated_in=5})
    return filesystem.make_directories(dir)
end

--- Create all parent directories for a given path.
-- @tparam string path The path whose parents should be created.
-- @return (true, nil) on success, (false, err) on failure.
-- @staticfct gears.filesystem.make_parent_directories
function filesystem.make_parent_directories(path)
    return make_directory(Gio.File.new_for_path(path):get_parent())
end

--- Check if a file exists, is readable and not a directory.
-- @tparam string filename The file path.
-- @treturn boolean True if file exists and is readable.
-- @staticfct gears.filesystem.file_readable
function filesystem.file_readable(filename)
    local gfile = Gio.File.new_for_path(filename)
    local gfileinfo = gfile:query_info("standard::type,access::can-read",
                                       Gio.FileQueryInfoFlags.NONE)
    return gfileinfo and gfileinfo:get_file_type() ~= "DIRECTORY" and
        gfileinfo:get_attribute_boolean("access::can-read")
end

--- Check if a file exists, is executable and not a directory.
-- @tparam string filename The file path.
-- @treturn boolean True if file exists and is executable.
-- @staticfct gears.filesystem.file_executable
function filesystem.file_executable(filename)
    local gfile = Gio.File.new_for_path(filename)
    local gfileinfo = gfile:query_info("standard::type,access::can-execute",
                                       Gio.FileQueryInfoFlags.NONE)
    return gfileinfo and gfileinfo:get_file_type() ~= "DIRECTORY" and
        gfileinfo:get_attribute_boolean("access::can-execute")
end

--- Check if a path exists, is readable and a directory.
-- @tparam string path The directory path.
-- @treturn boolean True if path exists and is readable.
-- @staticfct gears.filesystem.dir_readable
function filesystem.dir_readable(path)
    local gfile = Gio.File.new_for_path(path)
    local gfileinfo = gfile:query_info("standard::type,access::can-read",
                                       Gio.FileQueryInfoFlags.NONE)
    return gfileinfo and gfileinfo:get_file_type() == "DIRECTORY" and
        gfileinfo:get_attribute_boolean("access::can-read")
end

--- Check if a path is a directory.
-- @tparam string path The directory path
-- @treturn boolean True if path exists and is a directory.
-- @staticfct gears.filesystem.is_dir
function filesystem.is_dir(path)
    return Gio.File.new_for_path(path):query_file_type({}) == "DIRECTORY"
end

--- Get the config home according to the XDG basedir specification.
-- @return the config home (XDG_CONFIG_HOME) with a slash at the end.
-- @staticfct gears.filesystem.get_xdg_config_home
function filesystem.get_xdg_config_home()
    return (os.getenv("XDG_CONFIG_HOME") or os.getenv("HOME") .. "/.config") .. "/"
end

--- Get the cache home according to the XDG basedir specification.
-- @return the cache home (XDG_CACHE_HOME) with a slash at the end.
-- @staticfct gears.filesystem.get_xdg_cache_home
function filesystem.get_xdg_cache_home()
    return (os.getenv("XDG_CACHE_HOME") or os.getenv("HOME") .. "/.cache") .. "/"
end

--- Get the data home according to the XDG basedir specification.
-- @treturn string the data home (XDG_DATA_HOME) with a slash at the end.
-- @staticfct gears.filesystem.get_xdg_data_home
function filesystem.get_xdg_data_home()
    return (os.getenv("XDG_DATA_HOME") or os.getenv("HOME") .. "/.local/share") .. "/"
end

--- Get the data dirs according to the XDG basedir specification.
-- @treturn table the data dirs (XDG_DATA_DIRS) with a slash at the end of each entry.
-- @staticfct gears.filesystem.get_xdg_data_dirs
function filesystem.get_xdg_data_dirs()
    local xdg_data_dirs = os.getenv("XDG_DATA_DIRS") or "/usr/share:/usr/local/share"
    return gtable.map(
        function(dir) return dir .. "/" end,
        gstring.split(xdg_data_dirs, ":"))
end

--- Get the path to the user's config dir.
-- This is the directory containing the configuration file ("rc.lua").
-- @return A string with the requested path with a slash at the end.
-- @staticfct gears.filesystem.get_configuration_dir
function filesystem.get_configuration_dir()
    return awesome.conffile:match(".*/") or "./"
end

--- Get the path to a directory that should be used for caching data.
-- @return A string with the requested path with a slash at the end.
-- @staticfct gears.filesystem.get_cache_dir
function filesystem.get_cache_dir()
    local result = filesystem.get_xdg_cache_home() .. "awesome/"
    filesystem.make_directories(result)
    return result
end

--- Get the path to the directory where themes are installed.
-- @return A string with the requested path with a slash at the end.
-- @staticfct gears.filesystem.get_themes_dir
function filesystem.get_themes_dir()
    return (os.getenv('AWESOME_THEMES_PATH') or awesome.themes_path) .. "/"
end

--- Get the path to the directory where our icons are installed.
-- @return A string with the requested path with a slash at the end.
-- @staticfct gears.filesystem.get_awesome_icon_dir
function filesystem.get_awesome_icon_dir()
    return (os.getenv('AWESOME_ICON_PATH') or awesome.icon_path) .. "/"
end

--- Get the user's config or cache dir.
-- It first checks XDG_CONFIG_HOME / XDG_CACHE_HOME, but then goes with the
-- default paths.
-- @param d The directory to get (either "config" or "cache").
-- @return A string containing the requested path.
-- @staticfct gears.filesystem.get_dir
function filesystem.get_dir(d)
    if d == "config" then
        -- No idea why this is what is returned, I recommend everyone to use
        -- get_configuration_dir() instead
        require("gears.debug").deprecate("gears.filesystem.get_xdg_config_home() .. 'awesome/'", {deprecated_in=5})
        return filesystem.get_xdg_config_home() .. "awesome/"
    elseif d == "cache" then
        require("gears.debug").deprecate("gears.filesystem.get_cache_dir", {deprecated_in=5})
        return filesystem.get_cache_dir()
    end
end

--- Get the name of a random file from a given directory.
-- @tparam string path The directory to search.
-- @tparam[opt] table exts Specific extensions to limit the search to. eg:`{ "jpg", "png" }`
--   If ommited, all files are considered.
-- @tparam[opt=false] boolean absolute_path Return the absolute path instead of the filename.
-- @treturn string|nil A randomly selected filename from the specified path (with
--   a specified extension if required) or nil if no suitable file is found. If `absolute_path`
--   is set, then a path is returned instead of a file name.
-- @staticfct gears.filesystem.get_random_file_from_dir
function filesystem.get_random_file_from_dir(path, exts, absolute_path)
    local files, valid_exts = {}, {}

    -- Transforms { "jpg", ... } into { [jpg] = #, ... }
    if exts then for i, j in ipairs(exts) do valid_exts[j:lower():gsub("^[.]", "")] = i end end

    -- Build a table of files from the path with the required extensions
    local file_list = Gio.File.new_for_path(path):enumerate_children("standard::*", 0)

    -- This will happen when the directory doesn't exist.
    if not file_list then return nil end

    for file in function() return file_list:next_file() end do
        if file:get_file_type() == "REGULAR" then
            local file_name = file:get_display_name()

            if not exts or valid_exts[file_name:lower():match(".+%.(.*)$") or ""] then
               table.insert(files, file_name)
            end
        end
    end

    if #files == 0 then return nil end

    -- Return a randomly selected filename from the file table
    local file = files[math.random(#files)]

    return absolute_path and (path:gsub("[/]*$", "") .. "/" .. file) or file
end

return filesystem
