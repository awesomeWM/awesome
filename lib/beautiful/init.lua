----------------------------------------------------------------------------
--- Theme library.
--
-- @author Damien Leone &lt;damien.leone@gmail.com&gt;
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Damien Leone, Julien Danjou
-- @themelib beautiful
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
local colorful = require("beautiful.colorful")

local beautiful = {
    xresources = xresources,
    theme_assets = theme_assets,
    gtk = gtk,
    colorful = colorful,
    mt = {}
}

-- Local data
local theme = {}
local descs = setmetatable({}, { __mode = 'k' })
local fonts = setmetatable({}, { __mode = 'v' })
local active_font


-- luacheck: max comment line length 300

--- Get GTK+3 theme variables from GtkStyleContext
-- @treturn table Key-value table with the following structure:
-- <table class='widget_list' border=1>
-- <tr style='font-weight: bold;'> <th align='center'>Result key</th> <th align='center'>StyleContext key</th> <th align='center'>StyleContext fallback #1</th> <th align='center'>StyleContext fallback #2</th> <th align='center'>GTK Widget fallback</th> </tr>
-- <tr> <td>`font_size`</td> <td></td> <td></td> <td></td> <td>Label font-size</td> </tr>
-- <tr> <td>`font_family`</td> <td></td> <td></td> <td></td> <td>Label font-family</td> </tr>
-- <tr> <td>`bg_color`</td> <td>`theme_bg_color`</td> <td></td> <td></td> <td>Window bg</td> </tr>
-- <tr> <td>`fg_color`</td> <td>`theme_fg_color`</td> <td></td> <td></td> <td>Window fg</td> </tr>
-- <tr> <td>`base_color`</td> <td>`theme_base_color`</td> <td></td> <td></td> <td>Entry bg</td> </tr>
-- <tr> <td>`text_color`</td> <td>`theme_text_color`</td> <td></td> <td></td> <td>Entry fg</td> </tr>
-- <tr> <td>`button_bg_color`</td> <td>`theme_button_bg_color`</td> <td>`theme_bg_color`</td> <td></td> <td>Button bg</td> </tr>
-- <tr> <td>`button_fg_color`</td> <td>`theme_button_fg_color`</td> <td>`theme_fg_color`</td> <td></td> <td>Button fg</td> </tr>
-- <tr> <td>`button_border_color`</td> <td></td> <td></td> <td></td> <td>Button border-color</td> </tr>
-- <tr> <td>`button_border_radius`</td> <td></td> <td></td> <td></td> <td>Button border-radius</td> </tr>
-- <tr> <td>`button_border_width`</td> <td></td> <td></td> <td></td> <td>Button border-top-width</td> </tr>
-- <tr> <td>`selected_bg_color`</td> <td>`theme_selected_bg_color`</td> <td></td> <td></td> <td>ToggleButton bg</td> </tr>
-- <tr> <td>`selected_fg_color`</td> <td>`theme_selected_fg_color`</td> <td></td> <td></td> <td>ToggleButton fg</td> </tr>
-- <tr> <td>`menubar_bg_color`</td> <td>`menubar_bg_color`</td> <td>`theme_bg_color`</td> <td></td> <td>HeaderBar bg</td> </tr>
-- <tr> <td>`menubar_fg_color`</td> <td>`menubar_fg_color`</td> <td>`theme_fg_color`</td> <td></td> <td>HeaderBar fg</td> </tr>
-- <tr> <td>`header_button_bg_color`</td> <td>`header_button_bg_color`</td> <td>`menubar_bg_color`</td> <td>`theme_bg_color`</td> <td>HeaderBar > Button bg</td> </tr>
-- <tr> <td>`header_button_fg_color`</td> <td>`header_button_fg_color`</td> <td>`menubar_fg_color`</td> <td>`theme_fg_color`</td> <td>HeaderBar > Button fg</td> </tr>
-- <tr> <td>`header_button_border_color`</td> <td></td> <td></td> <td></td> <td>HeaderBar > Button border-color</td> </tr>
-- <tr> <td>`error_color`</td> <td>`error_color`</td> <td>`error_bg_color`</td> <td></td> <td>destructive Button bg</td> </tr>
-- <tr> <td>`error_bg_color`</td> <td>`error_bg_color`</td> <td>`error_color`</td> <td></td> <td>destructive Button bg</td> </tr>
-- <tr> <td>`error_fg_color`</td> <td>`error_fg_color`</td> <td>`theme_selected_fg_color`</td> <td></td> <td>destructive Button fg</td> </tr>
-- <tr> <td>`warning_color`</td> <td>`warning_color`</td> <td>`warning_bg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`warning_bg_color`</td> <td>`warning_bg_color`</td> <td>`warning_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`warning_fg_color`</td> <td>`warning_fg_color`</td> <td>`theme_selected_fg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`success_color`</td> <td>`success_color`</td> <td>`success_bg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`success_bg_color`</td> <td>`success_bg_color`</td> <td>`success_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`success_fg_color`</td> <td>`success_fg_color`</td> <td>`theme_selected_fg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`tooltip_bg_color`</td> <td>`theme_tooltip_bg_color`</td> <td>`theme_bg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`tooltip_fg_color`</td> <td>`theme_tooltip_fg_color`</td> <td>`theme_fg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`osd_bg_color`</td> <td>`osd_bg`</td> <td>`theme_tooltip_bg_color`</td> <td>`theme_bg_color`</td> <td></td> </tr>
-- <tr> <td>`osd_fg_color`</td> <td>`osd_fg`</td> <td>`theme_tooltip_fg_color`</td> <td>`theme_fg_color`</td> <td></td> </tr>
-- <tr> <td>`osd_border_color`</td> <td>`osd_borders_color`</td> <td>`osd_fg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`wm_bg_color`</td> <td>`wm_bg`</td> <td>`menubar_bg_color`</td> <td>`theme_bg_color`</td> <td>HeaderBar bg</td> </tr>
-- <tr> <td>`wm_border_focused_color`</td> <td>`wm_border_focused`</td> <td>`theme_selected_bg_color`</td> <td></td> <td>ToggleButton bg</td> </tr>
-- <tr> <td>`wm_border_unfocused_color`</td> <td>`wm_border_unfocused`</td> <td>`wm_border`</td> <td>`menubar_bg_color`</td><td>HeaderBar bg</td> </tr>
-- <tr> <td>`wm_title_focused_color`</td> <td>`wm_title_focused`</td> <td>`wm_title`</td> <td>`theme_selected_fg_color`</td> <td>ToggleButton fg</td> </tr>
-- <tr> <td>`wm_title_unfocused_color`</td> <td>`wm_title_unfocused`</td> <td>`wm_unfocused_title`</td> <td>`menubar_fg_color`</td><td>HeaderBar fg</td> </tr>
-- <tr> <td>`wm_icons_focused_color`</td> <td>`wm_icons_focused`</td> <td>`wm_title_focused`</td> <td>`theme_selected_fg_color`</td> <td>ToggleButton fg</td> </tr>
-- <tr> <td>`wm_icons_unfocused_color`</td> <td>`wm_icons_unfocused`</td> <td>`wm_title_unfocused`</td> <td>`menubar_fg_color`</td> <td>HeaderBar fg</td> </tr>
-- </table>
--
-- @staticfct beautiful.gtk.get_theme_variables

--- The default font.
-- @beautiful beautiful.font
-- @param string

-- The default background color.
-- @beautiful beautiful.bg_normal
-- @param color

-- The default focused element background color.
-- @beautiful beautiful.bg_focus
-- @param color

-- The default urgent element background color.
-- @beautiful beautiful.bg_urgent
-- @param color

-- The default minimized element background color.
-- @beautiful beautiful.bg_minimize
-- @param color

-- The system tray background color.
-- Please note that only solid colors are currently supported.
-- @beautiful beautiful.bg_systray
-- @param color

-- The default focused element foreground (text) color.
-- @beautiful beautiful.fg_normal
-- @param color

-- The default focused element foreground (text) color.
-- @beautiful beautiful.fg_focus
-- @param color

-- The default urgent element foreground (text) color.
-- @beautiful beautiful.fg_urgent
-- @param color

-- The default minimized element foreground (text) color.
-- @beautiful beautiful.fg_minimize
-- @param color

--- The gap between clients.
-- @beautiful beautiful.useless_gap
-- @param[opt=0] number

--- The fallback border width.
-- @beautiful beautiful.border_width
-- @param number

--- The fallback border color.
-- @beautiful beautiful.border_color
-- @param color

--- The wallpaper path.
-- @beautiful beautiful.wallpaper
-- @tparam string|gears.surface wallpaper

-- The icon theme name.
-- It has to be a directory in `/usr/share/icons` or an XDG icon folder.
-- @beautiful beautiful.icon_theme
-- @param string

--- The Awesome icon path.
-- @beautiful beautiful.awesome_icon
-- @tparam string|gears.surface icon

--- The current theme path (if any)
-- @tfield string beautiful.theme_path
-- @param string

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
-- @staticfct beautiful.get_font
function beautiful.get_font(name)
    return load_font(name).description
end

--- Get a new font with merged attributes, based on another one.
--
-- See https://developer.gnome.org/pango/stable/pango-Fonts.html#pango-font-description-from-string.
-- @tparam string|Pango.FontDescription name The base font.
-- @tparam string merge Attributes that should be merged, e.g. "bold".
-- @treturn lgi.Pango.FontDescription
-- @staticfct beautiful.get_merged_font
function beautiful.get_merged_font(name, merge)
    local font = beautiful.get_font(name)
    merge = Pango.FontDescription.from_string(merge)
    local merged = font:copy_static()
    merged:merge(merge, true)
    return beautiful.get_font(merged:to_string())
end

--- Get the height of a font.
--
-- @param name Name of the font.
-- @staticfct beautiful.get_font_height
function beautiful.get_font_height(name)
    return load_font(name).height
end

--- Function that initializes the theme settings. Should be run at the
-- beginning of the awesome configuration file (normally rc.lua).
--
-- Example usages:
--
--    -- Using a table
--    beautiful.init({font = 'Monospace Bold 10'})
--
--    -- From a config file
--    beautiful.init("<path>/theme.lua")
--
-- Example "<path>/theme.lua" (see `05-awesomerc.md:Variable_definitions`):
--
--    theme = {}
--        theme.font = 'Monospace Bold 10'
--    return theme
--
-- Example using the return value:
--
--    local beautiful = require("beautiful")
--    if not beautiful.init("<path>/theme.lua") then
--        beautiful.init("<path>/.last.theme.lua") -- a known good fallback
--    end
--
-- @tparam string|table config The theme to load. It can be either the path to
--   the theme file (which should return a table) or directly a table
--   containing all the theme values.
-- @treturn true|nil True if successful, nil in case of error.
-- @staticfct beautiful.init
function beautiful.init(config)
    if config then
        local state, t_theme = nil, nil
        local homedir = os.getenv("HOME")

        -- If `config` is the path to a theme file, run this file,
        -- otherwise if it is a theme table, save it.
        local t_config = type(config)
        if t_config == 'string' then
            -- Expand the '~' $HOME shortcut
            config = config:gsub("^~/", homedir .. "/")
            local dir = Gio.File.new_for_path(config):get_parent()
            rawset(beautiful, "theme_path", dir and (dir:get_path().."/") or nil)
            theme = protected_call(dofile, config)
            t_theme = type(theme)
            state = t_theme == 'table' and next(theme)
        elseif t_config == 'table' then
            rawset(beautiful, "theme_path", nil)
            theme = config
            state = next(theme)
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
            rawset(beautiful, "theme_path", nil)
            theme = {}
            local file = t_config == 'string' and (" from: " .. config)
            local err = (file and t_theme == 'table' and "got an empty table" .. file)
                     or (file and t_theme ~= 'table' and "got a " .. t_theme .. file)
                     or (t_config == 'table' and "got an empty table")
                     or ("got a " .. t_config)
            return gears_debug.print_error("beautiful: error loading theme: " .. err)
        end
    else
        return gears_debug.print_error("beautiful: error loading theme: no theme specified")
    end
end

--- Get the current theme.
--
-- @treturn table The current theme table.
-- @staticfct beautiful.get
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
