---------------------------------------------------------------------------
--- (Deprecated) class module for icon lookup for menubar
--
-- @author Kazunobu Kuriyama
-- @copyright 2015 Kazunobu Kuriyama
-- @classmod menubar.icon_theme
---------------------------------------------------------------------------

-- This implementation is based on the specifications:
--  Icon Theme Specification 0.12
--  http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-0.12.html

local beautiful = require("beautiful")
local gfs = require("gears.filesystem")
local GLib = require("lgi").GLib
local index_theme = require("menubar.index_theme")

local ipairs = ipairs
local setmetatable = setmetatable
local string = string
local table = table
local math = math

local get_pragmatic_base_directories = function()
    local dirs = {}

    local dir = GLib.build_filenamev({GLib.get_home_dir(), ".icons"})
    if gfs.dir_readable(dir) then
        table.insert(dirs, dir)
    end

    dir = GLib.build_filenamev({GLib.get_user_data_dir(), "icons"})
    if gfs.dir_readable(dir) then
        table.insert(dirs, dir)
    end

    for _, v in ipairs(GLib.get_system_data_dirs()) do
        dir = GLib.build_filenamev({v, "icons"})
        if gfs.dir_readable(dir) then
            table.insert(dirs, dir)
        end
    end

    local need_usr_share_pixmaps = true
    for _, v in ipairs(GLib.get_system_data_dirs()) do
        dir = GLib.build_filenamev({v, "pixmaps"})
        if gfs.dir_readable(dir) then
            table.insert(dirs, dir)
        end
        if dir == "/usr/share/pixmaps" then
            need_usr_share_pixmaps = false
        end
    end

    dir = "/usr/share/pixmaps"
    if need_usr_share_pixmaps and gfs.dir_readable(dir) then
        table.insert(dirs, dir)
    end

    return dirs
end

local get_default_icon_theme_name = function()
    local icon_theme_names = { "Adwaita", "gnome", "hicolor" }
    for _, dir in ipairs(get_pragmatic_base_directories()) do
        for _, icon_theme_name in ipairs(icon_theme_names) do
            local filename = string.format("%s/%s/index.theme", dir, icon_theme_name)
            if gfs.file_readable(filename) then
                return icon_theme_name
            end
        end
    end
    return "hicolor"
end

local icon_theme = { mt = {} }

local index_theme_cache = {}

--- Class constructor of `icon_theme`
-- @deprecated menubar.icon_theme.new
-- @tparam string icon_theme_name Internal name of icon theme
-- @tparam table base_directories Paths used for lookup
-- @treturn table An instance of the class `icon_theme`
icon_theme.new = function(icon_theme_name, base_directories)
    icon_theme_name = icon_theme_name or beautiful.icon_theme or get_default_icon_theme_name()
    base_directories = base_directories or get_pragmatic_base_directories()

    local self = {}
    self.icon_theme_name = icon_theme_name
    self.base_directories = base_directories
    self.extensions = { "png", "svg", "xpm" }

    -- Instantiate index_theme (cached).
    if not index_theme_cache[self.icon_theme_name] then
        index_theme_cache[self.icon_theme_name] = {}
    end
    local cache_key = table.concat(self.base_directories, ':')
    if not index_theme_cache[self.icon_theme_name][cache_key] then
        index_theme_cache[self.icon_theme_name][cache_key] = index_theme(
            self.icon_theme_name,
            self.base_directories)
    end
    self.index_theme = index_theme_cache[self.icon_theme_name][cache_key]

    return setmetatable(self, { __index = icon_theme })
end

local directory_matches_size = function(self, subdirectory, icon_size)
    local kind, size, min_size, max_size, threshold = self.index_theme:get_per_directory_keys(subdirectory)

    if kind == "Fixed" then
        return icon_size == size
    elseif kind == "Scalable" then
        return icon_size >= min_size and icon_size <= max_size
    elseif kind == "Threshold" then
        return icon_size >= size - threshold and icon_size <= size + threshold
    end

    return false
end

local directory_size_distance = function(self, subdirectory, icon_size)
    local kind, size, min_size, max_size, threshold = self.index_theme:get_per_directory_keys(subdirectory)

    if kind == "Fixed" then
        return math.abs(icon_size - size)
    elseif kind == "Scalable" then
        if icon_size < min_size then
            return min_size - icon_size
        elseif icon_size > max_size then
            return icon_size - max_size
        end
        return 0
    elseif kind == "Threshold" then
        if icon_size < size - threshold then
            return min_size - icon_size
        elseif icon_size > size + threshold then
            return icon_size - max_size
        end
        return 0
    end

    return 0xffffffff -- Any large number will do.
end

local lookup_icon = function(self, icon_name, icon_size)
    local checked_already = {}
    for _, subdir in ipairs(self.index_theme:get_subdirectories()) do
        for _, basedir in ipairs(self.base_directories) do
            for _, ext in ipairs(self.extensions) do
                if directory_matches_size(self, subdir, icon_size) then
                    local filename = string.format("%s/%s/%s/%s.%s",
                                                   basedir, self.icon_theme_name, subdir,
                                                   icon_name, ext)
                    if gfs.file_readable(filename) then
                        return filename
                    else
                        checked_already[filename] = true
                    end
                end
            end
        end
    end

    local minimal_size = 0xffffffff -- Any large number will do.
    local closest_filename = nil
    for _, subdir in ipairs(self.index_theme:get_subdirectories()) do
        local dist = directory_size_distance(self, subdir, icon_size)
        if dist < minimal_size then
            for _, basedir in ipairs(self.base_directories) do
                for _, ext in ipairs(self.extensions) do
                    local filename = string.format("%s/%s/%s/%s.%s",
                                                   basedir, self.icon_theme_name, subdir,
                                                   icon_name, ext)
                    if not checked_already[filename] then
                        if gfs.file_readable(filename) then
                            closest_filename = filename
                            minimal_size = dist
                        end
                    end
                end
            end
        end
    end
    return closest_filename
end

local find_icon_path_helper  -- Gets called recursively.
find_icon_path_helper = function(self, icon_name, icon_size)
    local filename = lookup_icon(self, icon_name, icon_size)
    if filename then
        return filename
    end

    for _, parent in ipairs(self.index_theme:get_inherits()) do
        local parent_icon_theme = icon_theme(parent, self.base_directories)
        filename = find_icon_path_helper(parent_icon_theme, icon_name, icon_size)
        if filename then
            return filename
        end
    end

    return nil
end

local lookup_fallback_icon = function(self, icon_name)
    for _, dir in ipairs(self.base_directories) do
        for _, ext in ipairs(self.extensions) do
            local filename = string.format("%s/%s.%s",
                                           dir,
                                           icon_name, ext)
            if gfs.file_readable(filename) then
                return filename
            end
        end
    end
    return nil
end

---  Look up an image file based on a given icon name and/or a preferable size.
-- @deprecated menubar.icon_theme:find_icon_path
-- @tparam string icon_name Icon name to be looked up
-- @tparam number icon_size Prefereable icon size
-- @treturn string Absolute path to the icon file, or nil if not found
function icon_theme:find_icon_path(icon_name, icon_size)
    icon_size = icon_size or 16
    if not icon_name or icon_name == "" then
        return nil
    end

    local filename = find_icon_path_helper(self, icon_name, icon_size)
    if filename then
        return filename
    end

    if self.icon_theme_name ~= "hicolor" then
        filename = find_icon_path_helper(icon_theme("hicolor", self.base_directories), icon_name, icon_size)
        if filename then
            return filename
        end
    end

    return lookup_fallback_icon(self, icon_name)
end

icon_theme.mt.__call = function(_, ...)
    return icon_theme.new(...)
end

return setmetatable(icon_theme, icon_theme.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
