---------------------------------------------------------------------------
--- Math module for gears.
--
-- @utillib gears.math
---------------------------------------------------------------------------

local rtable = table
local bezier = require("gears.math.bezier")

local gmath = { bezier = bezier }

local function subset_mask_apply(mask, set)
    local ret = {}
    for i = 1, #set do
        if mask[i] then
            rtable.insert(ret, set[i])
        end
    end
    return ret
end

local function subset_next(mask)
    local i = 1
    while i <= #mask and mask[i] do
        mask[i] = false
        i = i + 1
    end

    if i <= #mask then
        mask[i] = 1
        return true
    end
    return false
end

--- Return all subsets of a specific set.
-- This function, giving a set, will return all subset it.
-- For example, if we consider a set with value { 10, 15, 34 },
-- it will return a table containing 2^n set:
-- { }, { 10 }, { 15 }, { 34 }, { 10, 15 }, { 10, 34 }, etc.
-- @param set A set.
-- @return A table with all subset.
-- @staticfct gears.math.subsets
function gmath.subsets(set)
    local mask = {}
    local ret = {}
    for i = 1, #set do mask[i] = false end

    -- Insert the empty one
    rtable.insert(ret, {})

    while subset_next(mask) do
        rtable.insert(ret, subset_mask_apply(mask, set))
    end
    return ret
end

--- Make i cycle.
-- @param t A length. Must be greater than zero.
-- @param i An absolute index to fit into #t.
-- @return An integer in (1, t) or nil if t is less than or equal to zero.
-- @staticfct gears.math.cycle
function gmath.cycle(t, i)
    if t < 1 then return end
    i = i % t
    if i == 0 then
        i = t
    end
    return i
end

--- Round a number to an integer.
-- @tparam number x
-- @treturn integer
-- @staticfct gears.math.round
function gmath.round(x)
    return math.floor(x + 0.5)
end

--- Return the sign of the number x
-- return 1 if x is positive, -1 if negative and 0 if x is 0
-- @tparam number x
-- @treturn integer
-- @staticfct gears.math.sign
function gmath.sign(x)
    if x > 0 then
        return 1
    elseif x < 0 then
        return -1
    else
        return 0
    end
end

return gmath
