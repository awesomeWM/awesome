----------------------------------------------------------------------------
--- Theme library.
--
-- @author Damien Leone &lt;damien.leone@gmail.com&gt;
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Damien Leone, Julien Danjou
-- @release @AWESOME_VERSION@
-- @module beautiful
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

local beautiful = { mt = {} }

-- Local data
local theme = {}
local descs = setmetatable({}, { __mode = 'k' })
local fonts = setmetatable({}, { __mode = 'v' })
local active_font

--- Load a font name
--
-- @param name Font name, which can be a string or a table
local function load_font(name)
    name = name or active_font
    if name and type(name) ~= "string" and descs[name] then
        name = descs[name]
    end
    if fonts[name] then
        return fonts[name]
    end

    -- Load new font
    local desc = Pango.FontDescription.from_string(name)
    local ctx = PangoCairo.font_map_get_default():create_context()

    -- Apply default values from the context (e.g. a default font size)
    desc:merge(ctx:get_font_description(), false)

    -- Calculate fond height
    local metrics = ctx:get_metrics(desc, nil)
    local height = math.ceil((metrics:get_ascent() + metrics:get_descent()) / Pango.SCALE)

    local font = { name = name, description = desc, height = height }
    fonts[name] = font
    descs[desc] = name
    return font
end

--- Set an active font
--
-- @param name The font
local function set_font(name)
    active_font = load_font(name).name
end

--- Get a font description
--
-- @param name The name of the font
function beautiful.get_font(name)
    return load_font(name).description
end

--- Get the heigh of a font
--
-- @param name Name of the font
function beautiful.get_font_height(name)
    return load_font(name).height
end

--- Init function, should be runned at the beginning of configuration file.
-- @tparam string|table config The theme to load. It can be either the path to
--                      the theme file (returning a table) or directly the table
--                      containing all the theme values.
function beautiful.init(config)
    if config then
        local success
        local homedir = os.getenv("HOME")

        -- If config is the path to the theme file,
        -- run this file,
        -- else if it is the theme table, save it
        if type(config) == 'string' then
            -- Expand the '~' $HOME shortcut
            config = config:gsub("^~/", homedir .. "/")
            success, theme = pcall(function() return dofile(config) end)
        elseif type(config) == 'table' then
            success = true
            theme = config
        end

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
            return print("E: beautiful: error loading theme file " .. config)
        end
    else
        return print("E: beautiful: error loading theme: no theme specified")
    end
end

--- Get the current theme.
--
-- @treturn table The current theme table.
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
