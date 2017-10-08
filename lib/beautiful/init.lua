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
local gears_dpi = require("gears.dpi")
local Gio = require("lgi").Gio
local protected_call = require("gears.protected_call")

local xresources = require("beautiful.xresources")
local theme_assets = require("beautiful.theme_assets")

local beautiful = {
    xresources = xresources,
    theme_assets = theme_assets,
    mt = {}
}

-- Local data
local theme = {}
local descs = setmetatable({}, { __mode = 'k' })
local fonts = setmetatable({}, { __mode = 'v' })
local active_font
local font_dpi

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

--- The default clients border width.
-- Note that only solid colors are supported.
-- @beautiful beautiful.border_normal

--- The focused client border width.
-- Note that only solid colors are supported.
-- @beautiful beautiful.border_focus

--- The marked clients border width.
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

--- Font DPI API
-- @section font_dpi

--- Get the global font DPI
--
-- @return user set value, or Xft.dpi, or the core DPI reported by the server
-- @see beautiful.set_font_dpi
function beautiful.get_font_dpi()
    if not font_dpi then
        -- Might not be present when run under unit tests
        if awesome and awesome.xrdb_get_value then
            font_dpi = tonumber(awesome.xrdb_get_value("", "Xft.dpi"))
        end
    end
    -- Following Keith Packard's whitepaper on Xft,
    -- https://keithp.com/~keithp/talks/xtc2001/paper/xft.html#sec-editing
    -- the proper fallback for Xft.dpi is the vertical DPI reported by
    -- the X server. Note that this can change, so we don't store it
    return font_dpi or gears_dpi.core_dpi()
end

--- Set the global font DPI
--
-- @tparam[opt] number|nil dpi The DPI to set the font DPI to,
--   or nil to autocompute
-- @return the new global font DPI
-- @see beautiful.get_font_dpi
function beautiful.set_font_dpi(dpi)
    font_dpi = dpi
    -- TODO signal
    return beautiful.get_font_dpi()
end

--- Get the font DPI for a specific screen
--
-- The screen font DPI is computed from the global font DPI, prorated to the
-- ratio of the screen pixel density to the pixel density of the primary screen.
-- On uniform DPI environments, this is a no-op, but on mixed-DPI environments
-- it ensures that font scaling is uniform across different DPIs.
-- (This approach follows the one used in Qt.)
--
-- @tparam[opt] screen scr The screen to get font DPI information about.
--     Defaults to the primary screen
function beautiful.screen_font_dpi(scr)
    scr = scr and screen[scr] or screen.primary
    return beautiful.get_font_dpi()*scr.dpi/screen.primary.dpi
end

--- @section end

--- Load a font from a string or a font description.
--
-- @see https://developer.gnome.org/pango/stable/pango-Fonts.html#pango-font-description-from-string
-- @tparam string|lgi.Pango.FontDescription name Font, which can be a
--   string or a lgi.Pango.FontDescription.
-- @tparam[opt] number dpi The DPI to compute the height at (defaults to the
--   global font DPI)
-- @treturn table A table with `name`, `description` and `height`.
local function load_font(name, dpi)
    name = name or active_font
    dpi = dpi or beautiful.get_font_dpi()
    if name and type(name) ~= "string" then
        if descs[name] then
            name = descs[name]
        else
            name = name:to_string()
        end
    end

    -- TODO if there's a lot of reloading due to queries with different DPIs
    -- in the mixed-DPI case, consider building the cache based on a combination
    -- of name and DPI
    local font = fonts[name]

    if not font then
        -- Load new font
        local ctx = PangoCairo.font_map_get_default():create_context()

        local desc = Pango.FontDescription.from_string(name)
        -- Apply default values from the context (e.g. a default font size)
        desc:merge(ctx:get_font_description(), false)

        font = { name = name, context = ctx, description = desc,
            height = nil, dpi = nil }

        descs[desc] = name
    end

    local ctx = font.context
    local desc = font.description
    local height = font.height

    if font.dpi ~= dpi or not height then
        ctx:set_resolution(dpi)

        -- Calculate font height.
        local metrics = ctx:get_metrics(desc, nil)
        height = math.ceil((metrics:get_ascent() + metrics:get_descent()) / Pango.SCALE)
        if height == 0 then
            height = desc:get_size() / Pango.SCALE
            gears_debug.print_warning(string.format(
            "beautiful.load_font: could not get height for '%s' (likely missing font), using %d.",
            name, height))
        end
        font.height = height
        font.dpi = dpi
    end

    fonts[name] = font
    return { name = font.name, description = desc, height = height }
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

--- Get the height of a font in device pixels at the given DPI.
--
-- @param name Name of the font
-- @tparam number dpi The DPI to load the font at
function beautiful.get_font_height_at_dpi(name, dpi)
    return load_font(name, dpi).height
end

--- Get the height of a font in device pixels for the given screen.
--
-- @param name Name of the font
-- @tparam[opt] screen scr Screen to get the DPI information from.
--   If unspecified, the global font DPI setting will be used.
function beautiful.get_font_height(name, scr)
    return load_font(name, beautiful.screen_font_dpi(scr)).height
end

--- Init function, should be runned at the beginning of configuration file.
-- @tparam string|table config The theme to load. It can be either the path to
--   the theme file (returning a table) or directly the table
--   containing all the theme values.
function beautiful.init(config)
    if config then
        local homedir = os.getenv("HOME")

        -- If `config` is the path to a theme file, run this file,
        -- otherwise if it is a theme table, save it.
        if type(config) == 'string' then
            -- Expand the '~' $HOME shortcut
            config = config:gsub("^~/", homedir .. "/")
            local dir = Gio.File.new_for_path(config):get_parent()
            beautiful.theme_path = dir and (dir:get_path().."/") or nil
            theme = protected_call(dofile, config)
        elseif type(config) == 'table' then
            theme = config
        end

        if theme then
            -- expand '~'
            if homedir then
                for k, v in pairs(theme) do
                    if type(v) == "string" then theme[k] = v:gsub("^~/", homedir .. "/") end
                end
            end

            if theme.font then set_font(theme.font) end
        else
            theme = {}
            return gears_debug.print_error("beautiful: error loading theme file " .. config)
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
