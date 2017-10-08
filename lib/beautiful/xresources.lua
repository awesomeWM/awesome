----------------------------------------------------------------------------
--- Library for getting xrdb data.
--
-- @author Yauhen Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2015 Yauhen Kirylau
-- @module beautiful.xresources
----------------------------------------------------------------------------

-- Grab environment
local awesome = awesome
local screen = screen
local round = require("gears.math").round
local gears_debug = require("gears.debug")

local xresources = {}

local fallback = {
  --black
  color0 = '#000000',
  color8 = '#465457',
  --red
  color1 = '#cb1578',
  color9 = '#dc5e86',
  --green
  color2 = '#8ecb15',
  color10 = '#9edc60',
  --yellow
  color3 = '#cb9a15',
  color11 = '#dcb65e',
  --blue
  color4 = '#6f15cb',
  color12 = '#7e5edc',
  --purple
  color5 = '#cb15c9',
  color13 = '#b75edc',
  --cyan
  color6 = '#15b4cb',
  color14 = '#5edcb4',
  --white
  color7 = '#888a85',
  color15 = '#ffffff',
  --
  background  = '#0e0021',
  foreground  = '#bcbcbc',
}

--- Get current base colorscheme from xrdb.
-- @treturn table Color table with keys 'background', 'foreground' and 'color0'..'color15'
function xresources.get_current_theme()
    local keys = { 'background', 'foreground' }
    for i=0,15 do table.insert(keys, "color"..i) end
    local colors = {}
    for _, key in ipairs(keys) do
        colors[key] = awesome.xrdb_get_value("", key)
        if not colors[key] then
            gears_debug.print_warning("beautiful: can't get colorscheme from xrdb (using fallback).")
            return fallback
        end
        if colors[key]:find("rgb:") then
            colors[key] = "#"..colors[key]:gsub("[a]?rgb:", ""):gsub("/", "")
        end
    end
    return colors
end


local function get_screen(s)
    return s and screen[s]
end

--- Get global or per-screen DPI value falling back to xrdb.
-- @tparam[opt] integer|screen s The screen.
-- @treturn number DPI value.
function xresources.get_dpi(s)
    s = get_screen(s)
    if s then
        gears_debug.deprecate("use s.dpi", {deprecated_in=5})
        return s.dpi
    else
        gears_debug.deprecate("use beautiful.get_font_dpi()", {deprecated_in=5})
        return require("beautiful").get_font_dpi()
    end
end


--- Set DPI for a given screen (defaults to global).
-- @tparam number dpi DPI value.
-- @tparam[opt] integer s Screen.
function xresources.set_dpi(dpi, s)
    s = get_screen(s)
    if s then
        gears_debug.deprecate("use s.dpi=", {deprecated_in=5})
        s.dpi = dpi
    else
        gears_debug.deprecate("use beautiful.set_font_dpi()", {deprecated_in=5})
        require("beautiful").set_font_dpi(dpi)
    end
end


--- Compute resulting size applying current DPI value (optionally per screen).
-- @tparam number size Size
-- @tparam[opt] integer|screen s The screen.
-- @treturn integer Resulting size (rounded to integer).
function xresources.apply_dpi(size, s)
    return round(size / 96 * xresources.get_dpi(s))
end

return xresources

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
