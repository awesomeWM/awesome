----------------------------------------------------------------------------
--- Theme library.
--
-- @author Damien Leone &lt;damien.leone@gmail.com&gt;
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Damien Leone, Julien Danjou
-- @module beautiful
----------------------------------------------------------------------------

-- Grab environment
local os = os
local pairs = pairs
local type = type
local dofile = dofile
local setmetatable = setmetatable
local lgi = require("lgi")
local Pango = lgi.Pango
local PangoCairo = lgi.PangoCairo
local gears_debug = require("gears.debug")
local Gio = require("lgi").Gio
local protected_call = require("gears.protected_call")

local xresources = require("beautiful.xresources")
local theme_assets = require("beautiful.theme_assets")
local gtk = require("beautiful.gtk")

local beautiful = {
    xresources = xresources,
    theme_assets = theme_assets,
    gtk = gtk,
    mt = {}
}

-- Local data
local theme = {}
local descs = setmetatable({}, { __mode = 'k' })
local fonts = setmetatable({}, { __mode = 'v' })
local active_font

--- The default font.
-- @beautiful beautiful.font

-- The default background color.
-- @beautiful beautiful.bg_normal

-- The default focused element background color.
-- @beautiful beautiful.bg_focus

-- The default urgent element background color.
-- @beautiful beautiful.bg_urgent

-- The default minimized element background color.
-- @beautiful beautiful.bg_minimize

-- The system tray background color.
-- Please note that only solid colors are currently supported.
-- @beautiful beautiful.bg_systray

-- The default focused element foreground (text) color.
-- @beautiful beautiful.fg_normal

-- The default focused element foreground (text) color.
-- @beautiful beautiful.fg_focus

-- The default urgent element foreground (text) color.
-- @beautiful beautiful.fg_urgent

-- The default minimized element foreground (text) color.
-- @beautiful beautiful.fg_minimize

--- The gap between clients.
-- @beautiful beautiful.useless_gap
-- @param[opt=0] number

--- The client border width.
-- @beautiful beautiful.border_width

--- The default clients border color.
-- Note that only solid colors are supported.
-- @beautiful beautiful.border_normal

--- The focused client border color.
-- Note that only solid colors are supported.
-- @beautiful beautiful.border_focus

--- The marked clients border color.
-- Note that only solid colors are supported.
-- @beautiful beautiful.border_marked

--- The wallpaper path.
-- @beautiful beautiful.wallpaper

-- The icon theme name.
-- It has to be a directory in `/usr/share/icons` or an XDG icon folder.
-- @beautiful beautiful.icon_theme

--- The Awesome icon path.
-- @beautiful beautiful.awesome_icon

--- The current theme path (if any)
-- @tfield string beautiful.theme_path

--- Load a font from a string or a font description.
--
-- @see https://developer.gnome.org/pango/stable/pango-Fonts.html#pango-font-description-from-string
-- @tparam string|lgi.Pango.FontDescription name Font, which can be a
--   string or a lgi.Pango.FontDescription.
-- @treturn table A table with `name`, `description` and `height`.
local function load_font(name)
    name = name or active_font
    if name and type(name) ~= "string" then
        if descs[name] then
            name = descs[name]
        else
            name = name:to_string()
        end
    end
    if fonts[name] then
        return fonts[name]
    end

    -- Load new font
    local desc = Pango.FontDescription.from_string(name)
    local ctx = PangoCairo.font_map_get_default():create_context()
    ctx:set_resolution(beautiful.xresources.get_dpi())

    -- Apply default values from the context (e.g. a default font size)
    desc:merge(ctx:get_font_description(), false)

    -- Calculate font height.
    local metrics = ctx:get_metrics(desc, nil)
    local height = math.ceil((metrics:get_ascent() + metrics:get_descent()) / Pango.SCALE)
    if height == 0 then
        height = desc:get_size() / Pango.SCALE
        gears_debug.print_warning(string.format(
            "beautiful.load_font: could not get height for '%s' (likely missing font), using %d.",
            name, height))
    end

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

--- Get a font description.
--
-- See https://developer.gnome.org/pango/stable/pango-Fonts.html#PangoFontDescription.
-- @tparam string|lgi.Pango.FontDescription name The name of the font.
-- @treturn lgi.Pango.FontDescription
function beautiful.get_font(name)
    return load_font(name).description
end

--- Get a new font with merged attributes, based on another one.
--
-- See https://developer.gnome.org/pango/stable/pango-Fonts.html#pango-font-description-from-string.
-- @tparam string|Pango.FontDescription name The base font.
-- @tparam string merge Attributes that should be merged, e.g. "bold".
-- @treturn lgi.Pango.FontDescription
function beautiful.get_merged_font(name, merge)
    local font = beautiful.get_font(name)
    merge = Pango.FontDescription.from_string(merge)
    local merged = font:copy_static()
    merged:merge(merge, true)
    return beautiful.get_font(merged:to_string())
end

--- Get the height of a font.
--
-- @param name Name of the font
function beautiful.get_font_height(name)
    return load_font(name).height
end

--- Init function, should be run at the beginning of configuration file.
-- @tparam string|table config The theme to load. It can be either the path to
--   the theme file (returning a table) or directly the table
--   containing all the theme values.
function beautiful.init(config)
    if config then
        local state, err = nil, nil
        local homedir = os.getenv("HOME")

        -- If `config` is the path to a theme file, run this file,
        -- otherwise if it is a theme table, save it.
        if type(config) == 'string' then
            -- Expand the '~' $HOME shortcut
            config = config:gsub("^~/", homedir .. "/")
            local dir = Gio.File.new_for_path(config):get_parent()
            beautiful.theme_path = dir and (dir:get_path().."/") or nil
            state, err = pcall(function() theme = dofile(config) end)
        elseif type(config) == 'table' then
            state = #config ~= 0
            theme = config
        end

        if state then
            -- expand '~'
            if homedir then
                for k, v in pairs(theme) do
                    if type(v) == "string" then theme[k] = v:gsub("^~/", homedir .. "/") end
                end
            end

            if theme.font then set_font(theme.font) end
            return true
        else
            theme = {}
            gears_debug.print_error("beautiful: error loading theme: " .. tostring(config))
            gears_debug.print_error("theme file error: " .. tostring(err))
            return false, err
        end
    else
        return gears_debug.print_error("beautiful: error loading theme: no theme specified")
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

function beautiful.mt:__newindex(k, v)
    theme[k] = v
end

-- Set the default font
set_font("sans 8")

return setmetatable(beautiful, beautiful.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
