----------------------------------------------------------------------------
--- Library for getting xrdb data.
--
-- @author Yauhen Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2015 Yauhen Kirylau
-- @submodule beautiful
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
        local color = awesome.xrdb_get_value("", key)
        if color then
            if color:find("rgb:") then
                color = "#"..color:gsub("[a]?rgb:", ""):gsub("/", "")
            end
        else
            gears_debug.print_warning(
                "beautiful: can't get colorscheme from xrdb for value '"..key.."' (using fallback)."
            )
            color = fallback[key]
        end
        colors[key] = color
    end
    return colors
end


local function get_screen(s)
    return s and screen[s]
end

--- Get global or per-screen DPI value falling back to xrdb.
--
-- This function is deprecated. Use `s.dpi` and avoid getting the DPI without
-- a screen.
--
-- @deprecated xresources.get_dpi
-- @tparam[opt] integer|screen s The screen.
-- @treturn number DPI value.

function xresources.get_dpi(s)
    s = get_screen(s)
    if s then
        return s.dpi
    end
    if not xresources.dpi then
        -- Might not be present when run under unit tests
        if awesome and awesome.xrdb_get_value then
            xresources.dpi = tonumber(awesome.xrdb_get_value("", "Xft.dpi"))
        end
        -- Following Keith Packard's whitepaper on Xft,
        -- https://keithp.com/~keithp/talks/xtc2001/paper/xft.html#sec-editing
        -- the proper fallback for Xft.dpi is the vertical DPI reported by
        -- the X server. This will generally be 96 on Xorg, unless the user
        -- has configured it differently
        if not xresources.dpi then
            if root then
                local mm_to_inch = 25.4
                local _, h = root.size()
                local _, hmm = root.size_mm()
                if hmm ~= 0 then
                    xresources.dpi = round(h*mm_to_inch/hmm)
                end
            end
        end
        -- ultimate fallback
        if not xresources.dpi then
            xresources.dpi = 96
        end
    end
    return xresources.dpi
end


--- Set DPI for a given screen (defaults to global).
-- @tparam number dpi DPI value.
-- @tparam[opt] integer s Screen.
function xresources.set_dpi(dpi, s)
    s = get_screen(s)
    if not s then
        xresources.dpi = dpi
    else
        s.dpi = dpi
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
