----------------------------------------------------------------------------
-- @author Damien Leone &lt;damien.leone@gmail.com&gt;
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Damien Leone, Julien Danjou
-- @release @AWESOME_VERSION@
----------------------------------------------------------------------------

-- Grab environment
local os = os
local print = print
local pcall = pcall
local pairs = pairs
local type = type
local dofile = dofile
local setmetatable = setmetatable
local util = require("awful.util")
local lgi = require("lgi")
local cairo = lgi.cairo
local Pango = lgi.Pango
local PangoCairo = lgi.PangoCairo
local capi =
{
    screen = screen,
    awesome = awesome
}

--- Theme library.
local beautiful = { mt = {} }

-- Local data
local theme = {}
local descs = setmetatable({}, { __mode = 'k' })
local fonts = setmetatable({}, { __mode = 'v' })
local active_font

local function load_font(name)
    name = name or active_font
    if name and type(name) ~= "string" and descs[name] then
        name = descs[name]
    end
    if fonts[name] then
        return fonts[name]
    end
    -- load new font
    local desc = Pango.FontDescription.from_string(name)

    local ctx = PangoCairo.font_map_get_default():create_context()
    local layout = Pango.Layout.new(ctx)
    layout:set_font_description(desc)

    local width, height = layout:get_pixel_size()
    local font = { name = name, description = desc, height = height }
    fonts[name] = font
    descs[desc] = name
    return font
end

local function set_font(name)
    active_font = load_font(name).name
end

function beautiful.get_font(name)
    return load_font(name).description
end

function beautiful.get_font_height(name)
    return load_font(name).height
end

--- Init function, should be runned at the beginning of configuration file.
-- @param path The theme file path.
function beautiful.init(path)
    if path then
        local success

        -- try and grab user's $HOME directory and expand '~'
        local homedir = os.getenv("HOME")
        path = path:gsub("^~/", homedir .. "/")

        success, theme = pcall(function() return dofile(path) end)

        if not success then
            return print("E: beautiful: error loading theme file " .. theme)
        elseif theme then
            -- expand '~'
            if homedir then
                for k, v in pairs(theme) do
                    if type(v) == "string" then theme[k] = v:gsub("^~/", homedir .. "/") end
                end
            end

            if theme.font then set_font(theme.font) end
        else
            return print("E: beautiful: error loading theme file " .. path)
        end
    else
        return print("E: beautiful: error loading theme: no path specified")
    end
end

--- Get the current theme.
-- @return The current theme table.
function beautiful.get()
    return theme
end

function beautiful.mt:__index(k)
    return theme[k]
end

-- Set the default font
set_font("sans 8")

return setmetatable(beautiful, beautiful.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
