---------------------------------------------------------------------------
--- (Deprecated) class module for parsing an index.theme file
--
-- @author Kazunobu Kuriyama
-- @copyright 2015 Kazunobu Kuriyama
-- @classmod menubar.index_theme
---------------------------------------------------------------------------

-- This implementation is based on the specifications:
--  Icon Theme Specification 0.12
--  http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-0.12.html

local ipairs = ipairs
local setmetatable = setmetatable
local string = string
local table = table
local io = io

-- index.theme groups
local ICON_THEME = "Icon Theme"
-- index.theme keys
local DIRECTORIES = "Directories"
local INHERITS = "Inherits"
-- per-directory subkeys
local TYPE = "Type"
local SIZE = "Size"
local MINSIZE = "MinSize"
local MAXSIZE = "MaxSize"
local THRESHOLD = "Threshold"

local index_theme = { mt = {} }

--- Class constructor of `index_theme`
-- @deprecated menubar.index_theme.new
-- @tparam table cls Metatable that will be used. Should always be `index_theme.mt`.
-- @tparam string icon_theme_name Internal name of icon theme
-- @tparam table base_directories Paths used for lookup
-- @treturn table An instance of the class `index_theme`
index_theme.new = function(cls, icon_theme_name, base_directories)
    local self = {}
    setmetatable(self, { __index = cls })

    -- Initialize the fields
    self.icon_theme_name = icon_theme_name
    self.base_directory = nil
    self[DIRECTORIES] = {}
    self[INHERITS] = {}
    self.per_directory_keys = {}

    -- base_directory
    local basedir = nil
    local handler = nil
    for _, dir in ipairs(base_directories) do
        basedir = dir .. "/" .. self.icon_theme_name
        handler = io.open(basedir .. "/index.theme", "r")
        if handler then
            -- Use the index.theme which is found first.
            break
        end
    end
    if not handler then
        return self
    end
    self.base_directory = basedir

    -- Parse index.theme.
    while true do
        local line = handler:read()
        if not line then
            break
        end

        local group_header = "^%[(.+)%]$"
        local group = line:match(group_header)
        if group then
            if group == ICON_THEME then
                while true do
                    local item = handler:read()
                    if not item then
                        break
                    end
                    if item:match(group_header) then
                        handler:seek("cur", -string.len(item) - 1)
                        break
                    end

                    local k, v = item:match("^(%w+)=(.*)$")
                    if k == DIRECTORIES or k == INHERITS then
                        string.gsub(v, "([^,]+),?", function(match)
                            table.insert(self[k], match)
                        end)
                    end
                end
            else
                -- This must be a 'per-directory keys' group
                local keys = {}

                while true do
                    local item = handler:read()
                    if not item then
                        break
                    end
                    if item:match(group_header) then
                        handler:seek("cur", -string.len(item) - 1)
                        break
                    end

                    local k, v = item:match("^(%w+)=(%w+)$")
                    if k == SIZE or k == MINSIZE or k == MAXSIZE or k == THRESHOLD then
                        keys[k] = tonumber(v)
                    elseif k == TYPE then
                        keys[k] = v
                    end
                end

                -- Size is a must.  Other keys are optional.
                if keys[SIZE] then
                    -- Set unset keys to the default values.
                    if not keys[TYPE] then keys[TYPE] = THRESHOLD end
                    if not keys[MINSIZE] then keys[MINSIZE] = keys[SIZE] end
                    if not keys[MAXSIZE] then keys[MAXSIZE] = keys[SIZE] end
                    if not keys[THRESHOLD] then keys[THRESHOLD] = 2 end

                    self.per_directory_keys[group] = keys
                end
            end
        end
    end

    handler:close()

    return self
end

--- Table of the values of the `Directories` key
-- @deprecated menubar.index_theme:get_subdirectories
-- @treturn table Values of the `Directories` key
index_theme.get_subdirectories = function(self)
    return self[DIRECTORIES]
end

--- Table of the values of the `Inherits` key
-- @deprecated menubar.index_theme:get_inherits
-- @treturn table Values of the `Inherits` key
index_theme.get_inherits = function(self)
    return self[INHERITS]
end

--- Query (part of) per-directory keys of a given subdirectory name.
-- @deprecated menubar.index_theme:get_per_directory_keys
-- @tparam table subdirectory Icon theme's subdirectory
-- @treturn string Value of the `Type` key
-- @treturn number Value of the `Size` key
-- @treturn number VAlue of the `MinSize` key
-- @treturn number Value of the `MaxSize` key
-- @treturn number Value of the `Threshold` key
function index_theme:get_per_directory_keys(subdirectory)
    local keys = self.per_directory_keys[subdirectory]
    return keys[TYPE], keys[SIZE], keys[MINSIZE], keys[MAXSIZE], keys[THRESHOLD]
end

index_theme.mt.__call = function(cls, icon_theme_name, base_directories)
    return index_theme.new(cls, icon_theme_name, base_directories)
end

return setmetatable(index_theme, index_theme.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
