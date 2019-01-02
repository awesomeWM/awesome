---------------------------------------------------------------------------
--- Math module for gears
--
-- @module gears.math
---------------------------------------------------------------------------

local rtable = table

local gmath = {}

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
-- @class function
-- @name subsets
-- @param set A set.
-- @return A table with all subset.
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
--
-- By default, this function only cycle forward. If it can be used with negative
-- `i`, set `symetric` to true.
--
-- @class function
-- @name cycle
-- @tparam number t A length. Must be greater than zero.
-- @tparam number i An absolute index to fit into #t.
-- @tparam[opt=false] boolean symetric Also cycle backward.
-- @treturn number|nil An integer in (1, t) or nil if t is less than or equal
--  to zero. It returns `nil` if `symetric` is false and `i` less than 1.
function gmath.cycle(t, i, symetric)
    if t < 1 and not symetric then return end

    i = i % t

    if i == 0 then
        i = t
    end
    return i
end

--- Round a number to an integer.
-- @class function
-- @name round
-- @tparam number x
-- @treturn integer
function gmath.round(x)
    return math.floor(x + 0.5)
end


return gmath
