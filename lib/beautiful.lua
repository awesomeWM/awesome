----------------------------------------------------------------------------
-- @author Damien Leone &lt;damien.leone@gmail.com&gt;
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Damien Leone, Julien Danjou
----------------------------------------------------------------------------

-- Grab environment
local io = io
local print = print
local setmetatable = setmetatable
local awful = require("awful")
local package = package
local capi = { awesome = awesome }

local module_name = "beautiful"

--- beautiful: theme library
module(module_name)

-- Local data
local theme = {}

--- Split line in two if it  contains the '=' character.
-- @param line The line to split.
-- @return nil if  the '=' character is not in the string
local function split_line(line)
    local split_val = line:find('=')

    if split_val and line:sub(1, 1) ~= '#' and line:sub(1, 2) ~= '--' then
        -- Remove spaces in key and extra spaces before value
        local value = line:sub(split_val + 1, -1)
        while value:sub(1, 1) == ' ' do
            value = value:sub(2, -1)
        end

        return line:sub(1, split_val - 1):gsub(' ', ''), value
    end
end

--- Get a value directly.
-- @param key The key.
-- @return The value.
function __index(self, key)
    if theme[key] then
        return theme[key]
    end
end

--- Init function, should be runned at the beginning of configuration file.
-- @param path The theme file path.
function init(path)
    if path then
       local f = io.open(path)

       if not f then
           return print("E: unable to load theme " .. path)
       end

       for line in f:lines() do
            local key, value
            key, value = split_line(line)
            if key then
                if key == "wallpaper_cmd" then
                    awful.spawn(value)
                elseif key == "font" then
                    capi.awesome.font_set(value)
                elseif key == "fg_normal" then
                    capi.awesome.colors_set({ fg = value })
                elseif key == "bg_normal" then
                    capi.awesome.colors_set({ bg = value })
                end
                -- Store.
                theme[key] = value
            end
        end
        f:close()
    end
end

setmetatable(package.loaded[module_name], package.loaded[module_name])

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
