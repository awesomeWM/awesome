---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local setmetatable = setmetatable
local ipairs = ipairs
local table = table
local error = error

-- gears.sort
local sort = { mt = {} }

local function less_than_comp(a, b)
    return a < b
end

--- Sort a table. This interface should be identical to table.sort().
-- The difference to table.sort() is that this sort is stable.
-- @param list The table to sort (we do an in-place sort!).
-- @param comp Comparator used for the sorting
function sort.sort(list, comp)
    local comp = comp or less_than_comp

    -- A table could contain non-integer keys which we have to ignore.
    local num = 0
    for k, v in ipairs(list) do
        num = num + 1
    end

    if num <= 1 then
        -- Nothing to do
        return
    end

    -- Sort until everything is sorted :)
    local sorted = false
    local n = num
    while not sorted do
        sorted = true
        for i = 1, n - 1 do
            -- Two equal elements won't be swapped -> we are stable
            if comp(list[i+1], list[i]) then
                local tmp = list[i]
                list[i] = list[i+1]
                list[i+1] = tmp

                sorted = false
            end
        end
        -- The last element is now guaranteed to be in the right spot
        n = n - 1
    end
end

function sort.test()
    local function test_one(t)
        local len = #t
        sort.sort(t)
        if len ~= #t then
            error("Table lost entries during sort!")
        end
        if len > 1 then
            local cur = table.remove(t, 1)
            len = len - 1
            while len > 0 do
                local next = table.remove(t, 1)
                if cur > next then
                    error("Table not sorted!")
                end
                cur = next
                len = len -1
            end
        end
    end
    test_one({1, 2, 3, 4, 5, 6})
    test_one({6, 5, 4, 3, 2, 1})
    return true
end

function sort.mt:__call(...)
    return sort.sort(...)
end

return setmetatable(sort, sort.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
