----------------------------------------------------------------------------
--- Library for getting xrdb data.
--
-- @author Yauhen Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2015 Yauhen Kirylau
-- @release @AWESOME_VERSION@
-- @module beautiful.xresources
----------------------------------------------------------------------------

-- Grab environment
local print = print
local awesome = awesome

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
            print("W: beautiful: can't get colorscheme from xrdb (using fallback).")
            return fallback
        end
    end
    return colors
end


local dpi_per_screen = {}

--- Get global or per-screen DPI value falling back to xrdb.
-- @tparam[opt=focused] integer s The screen.
-- @treturn number DPI value.
function xresources.get_dpi(s)
    if not s or not dpi_per_screen[s] then
        if not xresources.dpi then
            xresources.dpi = tonumber(awesome.xrdb_get_value("", "Xft.dpi") or 96)
        end
        return xresources.dpi
    end
    return dpi_per_screen[s]
end


--- Set DPI for a given screen (defaults to global).
-- @tparam number dpi DPI value.
-- @tparam[opt] integer s Screen.
function xresources.set_dpi(dpi, s)
    if not s then
        xresources.dpi = dpi
    else
        dpi_per_screen[s] = dpi
    end
end


--- Compute resulting size applying current DPI value (optionally per screen).
-- @tparam number size Size
-- @tparam[opt] integer s The screen.
-- @treturn number Resulting size
function xresources.apply_dpi(size, s)
    return size/96*xresources.get_dpi(s)
end

return xresources

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
