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

local xresources = {}

local fallback = {
  --black
  ["0"] = '#000000',
  ["8"] = '#465457',
  --red
  ["1"] = '#cb1578',
  ["9"] = '#dc5e86',
  --green
  ["2"] = '#8ecb15',
  ["10"] = '#9edc60',
  --yellow
  ["3"] = '#cb9a15',
  ["11"] = '#dcb65e',
  --blue
  ["4"] = '#6f15cb',
  ["12"] = '#7e5edc',
  --purple
  ["5"] = '#cb15c9',
  ["13"] = '#b75edc',
  --cyan
  ["6"] = '#15b4cb',
  ["14"] = '#5edcb4',
  --white
  ["7"] = '#888a85',
  ["15"] = '#ffffff',
  --
  bg  = '#0e0021',
  fg  = '#bcbcbc',
}

--- Get current base colorscheme from xrdb.
-- @treturn table Color table with keys 'bg', 'fg' and '0'..'15'
function xresources.get_current_theme()
    local colors = {}
    local output = io.popen("xrdb -query")
    local query = output:read('*a')
    output:close()
    for i,color in string.gmatch(query, "%*color(%d+):[^#]*(#[%a%d]+)") do
        colors[i] = color
    end
    if not colors["15"] then
        colors = fallback
        print("W: beautiful: can't get colorscheme from xrdb (using fallback):")
        print(query)
    end
    colors.bg = string.match(query, "*background:[^#]*(#[%a%d]+)") or fallback.bg
    colors.fg = string.match(query, "*foreground:[^#]*(#[%a%d]+)") or fallback.fg
    return colors
end


--- Get DPI value from xsettingsd or xrdb as a fallback.
-- @treturn number DPI value.
function xresources.get_dpi()

    local function get_dpi()
        local function get_dpi_common(exec, key)
            local output = io.popen(exec)
            local query = output:read('*a')
            local dpi = tonumber(string.match(query, key.."[%s]+([%d]+)"))
            output:close()
            return dpi
        end
        local dpi = get_dpi_common("dump_xsettings", "Xft/DPI")
        if dpi then return dpi/1024 end
        print("W: beautiful: can't get dpi from xsettingsd (using xrdb)")
        dpi = get_dpi_common("xrdb -query", "dpi:")
        if dpi then return dpi end
        print("E: beautiful: can't get dpi from xrdb (using default value)")
        return 96
    end

    if not xresources.dpi then
        xresources.dpi = get_dpi()
    end
    return xresources.dpi
end


--- Compute resulting size applying current DPI value.
-- @tparam number size Size
-- @treturn number Resulting size
function xresources.apply_dpi(size)
    return size/96*xresources.get_dpi()
end

return xresources

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
