---------------------------------------------------------------------------
--
-- Auxiliary DPI-related functions.
--
-- @author Giuseppe Bilotta
-- @copyright 2017 Giuseppe Bilotta
-- @module gears.dpi
---------------------------------------------------------------------------

local mround = require("gears.math").round

local gdpi = {}

local gdpi_mt = {}

-- DPI rounding policy
--
-- Most monitors don't have a DPI which is a nice exact multiple of the
-- reference pixel density (96). In such cases, users' preferences vary:
-- some prefer using the DPI values as-is, resulting in fractional scaling,
-- others prefer rounding the scaling factor (so that e.g. anything above 144
-- DPI and below 240 should result in a scaling factor of 2).
-- While this could be achieved by the users by manually forcing specific DPI
-- values for the screens, it's rather annoying in dynamic configurations.
-- So we offer the possibility for the user to choose a global policy,
-- and all DPI usage will take this into consideration.
-- Note that rounding is only applied to autocomputed DPI values, never
-- when the user sets the DPI themselves.

--- Predefined DPI rounding policies
--
-- maps string names to rounding function.
local round_table = {
    identity = function(dpi) return dpi end, -- identity/passthrough/no rounding
    floor =  function(dpi) return math.floor(dpi/96)*96 end, -- floor
    ceil =  function(dpi) return math.ceil(dpi/96)*96 end, -- ceiling
    round =  function(dpi) return mround(dpi/96)*96 end, -- round-to-nearest
    user = nil -- user-defined function
}

local rounding_policy = 'identity'

--- Round a DPI value according to the current DPI rounding policy
-- @tparam number dpi raw DPI value
-- @treturn number rounded DPI value
function gdpi.round(dpi)
    return round_table[rounding_policy](dpi)
end

--- Set the DPI rounding policy
--
-- @property rounding_policy
-- @tparam string|function policy Policy name (identity, floor, ceil, round)
--   or user function, or nil to reset to default (identity)
local function set_rounding_policy(policy)
    policy = policy or 'identity'
    if type(policy) == 'function' then
        round_table.user = policy
        policy = 'user'
    elseif type(policy) ~= 'string' then
        local allowed = ""
        for k, _ in pairs(round_table) do
            allowed = allowed .. k .. ", "
        end
        error("DPI rounding policy must be one of " .. allowed ..
              " a function or nil")
    elseif policy == 'user' and not round_table.user then
        error("no user DPI rounding policy defined")
    elseif not round_table[policy] then
        error("no such DPI rounding policy '" .. policy .. "'")
    end

    rounding_policy = policy
end

function gdpi_mt.__index(_, key)
    if key ~= 'rounding_policy' then
        error("unknown gears.dpi property " .. key)
    end
    return rounding_policy
end

function gdpi_mt.__newindex(_, key, value)
    if key ~= 'rounding_policy' then
        error("unknown gears.dpi property " .. key)
    end
    return set_rounding_policy(value)
end


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
            dpi = gdpi.round(h*mm_to_inch/hmm)
        end
    end
    return dpi
end

return setmetatable(gdpi, gdpi_mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
