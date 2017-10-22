---------------------------------------------------------------------------
--
-- Auxiliary DPI-related functions.
--
-- @author Giuseppe Bilotta
-- @copyright 2017 Giuseppe Bilotta
-- @module gears.dpi
---------------------------------------------------------------------------

local gdpi = {}

--- Get core DPI
-- Return the vertical DPI reported by the X server via the core protocol
--
-- For servers with the RANDR extensions, this can change dynamically, so it's
-- better not to cache it
-- @treturn number the reported DPI, with a fallback of 96
function gdpi.core_dpi()
    local dpi = 96 -- default
    if root then
        local mm_to_inch = 25.4
        local _, h = root.size()
        local _, hmm = root.size_mm()
        if hmm ~= 0 then
            dpi = h*mm_to_inch/hmm
        end
    end
    return dpi
end

return gdpi
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
