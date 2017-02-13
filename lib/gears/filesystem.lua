---------------------------------------------------------------------------
--- Filesystem module for gears
--
-- @module gears.filesystem
---------------------------------------------------------------------------

-- Grab environment we need
local Gio = require("lgi").Gio

local filesystem = {}

--- Create a directory
-- @tparam string dir The directory.
-- @return (true, nil) on success, (false, err) on failure
function filesystem.mkdir(dir)
	local gfile = Gio.File.new_for_path(dir)
	local success, err = gfile:make_directory_with_parents()
	if success then
		return true
	end
	if err.domain == Gio.IOErrorEnum and err.code == "EXISTS" then
		-- Direcotry already exists, let this count as success
		return true
	end
	return false, err
end

--- Check if a file exists, is readable and not a directory.
-- @tparam string filename The file path.
-- @treturn boolean True if file exists and is readable.
function filesystem.file_readable(filename)
    local gfile = Gio.File.new_for_path(filename)
    local gfileinfo = gfile:query_info("standard::type,access::can-read",
                                       Gio.FileQueryInfoFlags.NONE)
    return gfileinfo and gfileinfo:get_file_type() ~= "DIRECTORY" and
        gfileinfo:get_attribute_boolean("access::can-read")
end

--- Check if a path exists, is readable and a directory.
-- @tparam string path The directory path.
-- @treturn boolean True if path exists and is readable.
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
function filesystem.is_dir(path)
    return Gio.File.new_for_path(path):query_file_type({}) == "DIRECTORY"
end

--- Get the config home according to the XDG basedir specification.
-- @return the config home (XDG_CONFIG_HOME) with a slash at the end.
function filesystem.get_xdg_config_home()
    return (os.getenv("XDG_CONFIG_HOME") or os.getenv("HOME") .. "/.config") .. "/"
end

--- Get the cache home according to the XDG basedir specification.
-- @return the cache home (XDG_CACHE_HOME) with a slash at the end.
function filesystem.get_xdg_cache_home()
    return (os.getenv("XDG_CACHE_HOME") or os.getenv("HOME") .. "/.cache") .. "/"
end

--- Get the path to the user's config dir.
-- This is the directory containing the configuration file ("rc.lua").
-- @return A string with the requested path with a slash at the end.
function filesystem.get_configuration_dir()
    return awesome.conffile:match(".*/") or "./"
end

--- Get the path to a directory that should be used for caching data.
-- @return A string with the requested path with a slash at the end.
function filesystem.get_cache_dir()
    return filesystem.get_xdg_cache_home() .. "awesome/"
end

--- Get the path to the directory where themes are installed.
-- @return A string with the requested path with a slash at the end.
function filesystem.get_themes_dir()
    return (os.getenv('AWESOME_THEMES_PATH') or awesome.themes_path) .. "/"
end

--- Get the path to the directory where our icons are installed.
-- @return A string with the requested path with a slash at the end.
function filesystem.get_awesome_icon_dir()
    return (os.getenv('AWESOME_ICON_PATH') or awesome.icon_path) .. "/"
end

--- Get the user's config or cache dir.
-- It first checks XDG_CONFIG_HOME / XDG_CACHE_HOME, but then goes with the
-- default paths.
-- @param d The directory to get (either "config" or "cache").
-- @return A string containing the requested path.
function filesystem.get_dir(d)
    if d == "config" then
        -- No idea why this is what is returned, I recommend everyone to use
        -- get_configuration_dir() instead
        return filesystem.get_xdg_config_home() .. "awesome/"
    elseif d == "cache" then
        return filesystem.get_cache_dir()
    end
end

return filesystem
