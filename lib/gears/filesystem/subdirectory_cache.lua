---------------------------------------------------------------------------
-- Check if some file exists inside a directory.
--
-- For performance reasons this module once recursively scans the directory to
-- collect information about all files and then uses this cache to answer
-- queries. If the directory is modified, it is rescanned. However,
-- subdirectories are not checked for modification.
--
-- Note that this is not re-entrant in the sense that you may not use instances
-- of this class from multiple async contexts at once.
--
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
-- @classmod gears.filesystem.subdirectory_cache
---------------------------------------------------------------------------

local gio = require("lgi").Gio
local gdebug = require("gears.debug")

local cache = {}

local function async_get_mtime(file)
    local info = file:async_query_info("time::modified", gio.FileQueryInfoFlags.NONE)
    return info and info:get_attribute_uint64("time::modified")
end

local function get_readable_path(gfile)
    return gfile:get_path() or gfile:get_uri()
end

local function scan(self, directory)
    local query = "standard::name,standard::type"
    local enum, err = directory:async_enumerate_children(query, gio.FileQueryInfoFlags.NONE)
    if not enum then
        gdebug.print_warning(get_readable_path(directory) .. ": " .. tostring(err))
        return
    end
    local per_call = 1000 -- Random value
    local children = {}
    while true do
        local list, enum_err = enum:async_next_files(per_call)
        if enum_err then
            gdebug.print_warning(get_readable_path(directory) .. ": " .. tostring(enum_err))
            return
        end
        for _, info in ipairs(list) do
            local file_type = info:get_file_type()
            local child = enum:get_child(info)
            if file_type == 'REGULAR' then
                local path = child:get_path()
                if path then
                    self.known_paths[path] = true
                end
            elseif file_type == 'DIRECTORY' then
                table.insert(children, child)
            end
        end
        if #list == 0 then
            break
        end
    end
    enum:async_close()
    for _, child in ipairs(children) do
        scan(self, child)
    end
end

--- Asynchronously ensure that this cache object is up to date. You do not have
-- to call this yourself unless you want to inspect the `.known_paths` table
-- directly.
function cache:async_update()
    local now = os.time()

    -- Can we still just assume that we are up to date?
    if self.last_check + self.timeout >= now then
        return
    end
    self.last_check = now

    -- Did the directory change?
    local mtime = async_get_mtime(self.directory)
    if self.directory_mtime == mtime then
        return
    end
    self.directory_mtime = mtime

    -- We need to rebuild the cache
    self.known_paths = {}
    scan(self, self.directory)
end

--- Asynchronously check if a file exists.
-- @tparam string path relative path of the file.
-- @treturn boolean True if the file exists
function cache:async_check_exists(path)
    local sane_path = self.directory:get_child(path):get_path()
    if not sane_path then
        return false
    end
    self:async_update()
    return self.known_paths[sane_path] or false
end

--- Asynchronously creates a new cache object.
-- @tparam string directory The directory to check.
-- @tparam[opt=5] int timeout Number of seconds that cache might be out of date.
-- @return A new cache object.
function cache.async_new(directory, timeout)
    local result = setmetatable({
        directory = gio.File.new_for_path(directory),
        timeout = timeout or 5,
        last_check = 0,
        directory_mtime = 0,
        known_paths = {},
    }, { __index = cache })
    result:async_update()
    return result
end

--- Contains all known subfiles
-- @table cache.known_paths

return cache

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
