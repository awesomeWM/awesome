---------------------------------------------------------------------------
--- Table module for gears
--
-- @module gears.table
---------------------------------------------------------------------------


local rtable = table

local gmath = require("gears.math")
local gtable = {}

--- Join all tables given as parameters.
-- This will iterate all tables and insert all their keys into a new table.
-- @class function
-- @name join
-- @param args A list of tables to join
-- @return A new table containing all keys from the arguments.
function gtable.join(...)
    local ret = {}
    for i = 1, select("#", ...) do
        local t = select(i, ...)
        if t then
            for k, v in pairs(t) do
                if type(k) == "number" then
                    rtable.insert(ret, v)
                else
                    ret[k] = v
                end
            end
        end
    end
    return ret
end

--- Override elements in the first table by the one in the second.
--
-- Note that this method doesn't copy entries found in `__index`.
-- @class function
-- @name crush
-- @tparam table t the table to be overriden
-- @tparam table set the table used to override members of `t`
-- @tparam[opt=false] boolean raw Use rawset (avoid the metatable)
-- @treturn table t (for convenience)
function gtable.crush(t, set, raw)
    if raw then
        for k, v in pairs(set) do
            rawset(t, k, v)
        end
    else
        for k, v in pairs(set) do
            t[k] = v
        end
    end

    return t
end

--- Pack all elements with an integer key into a new table
-- While both lua and luajit implement __len over sparse
-- table, the standard define it as an implementation
-- detail.
--
-- This function remove any non numeric keys from the value set
--
-- @class function
-- @name from_sparse
-- @tparam table t A potentially sparse table
-- @treturn table A packed table with all numeric keys
function gtable.from_sparse(t)
    local keys= {}
    for k in pairs(t) do
        if type(k) == "number" then
            keys[#keys+1] = k
        end
    end

    table.sort(keys)

    local ret = {}
    for _,v in ipairs(keys) do
        ret[#ret+1] = t[v]
    end

    return ret
end

--- Check if a table has an item and return its key.
--
-- If the input table only has continuous integer keys, consider setting
-- `ordered` to make your code more predictable and easier to test.
--
-- @class function
-- @name hasitem
-- @tparam table t The table.
-- @param item The item to look for in values of the table.
-- @tparam[opt=false] boolean ordered For indexed table, iterate in a
--  predictable order.
-- @tparam[opt=1] number first_key Where to begin looking.
-- @return The key were the item is found, or `nil` if not found.
function gtable.hasitem(t, item, ordered, first_key)
    if not ordered then
        for k, v in pairs(t) do
            if v == item then
                return k
            end
        end
    else
        for k = first_key or 1, #t do
            if t[k] == item then
                return k
            end
        end
    end
end

--- Get a sorted table with all integer keys from a table
-- @class function
-- @name keys
-- @param t the table for which the keys to get
-- @return A table with keys
function gtable.keys(t)
    local keys = { }
    for k, _ in pairs(t) do
        rtable.insert(keys, k)
    end
    rtable.sort(keys, function (a, b)
        return type(a) == type(b) and a < b or false
    end)
    return keys
end

--- Filter a tables keys for certain content types
-- @class function
-- @name keys_filter
-- @param t The table to retrieve the keys for
-- @param ... the types to look for
-- @return A filtered table with keys
function gtable.keys_filter(t, ...)
    local keys = gtable.keys(t)
    local keys_filtered = { }
    for _, k in pairs(keys) do
        for _, et in pairs({...}) do
            if type(t[k]) == et then
                rtable.insert(keys_filtered, k)
                break
            end
        end
    end
    return keys_filtered
end

--- Reverse a table
-- @class function
-- @name reverse
-- @param t the table to reverse
-- @return the reversed table
function gtable.reverse(t)
    local tr = { }
    -- reverse all elements with integer keys
    for _, v in ipairs(t) do
        rtable.insert(tr, 1, v)
    end
    -- add the remaining elements
    for k, v in pairs(t) do
        if type(k) ~= "number" then
            tr[k] = v
        end
    end
    return tr
end

--- Clone a table
-- @class function
-- @name clone
-- @param t the table to clone
-- @param deep Create a deep clone? (default: true)
-- @return a clone of t
function gtable.clone(t, deep)
    deep = deep == nil and true or deep
    local c = { }
    for k, v in pairs(t) do
        if deep and type(v) == "table" then
            c[k] = gtable.clone(v)
        else
            c[k] = v
        end
    end
    return c
end


--- Returns an iterator to cycle through, starting from the first element or the
-- given index, all elements of a table that match a given criteria.
--
-- @class function
-- @name iterate
-- @tparam table t The table to iterate
-- @tparam[opt=nil] function filter a function that returns true to indicate a
--  positive match.
-- @tparam number start  what index to start iterating from.  Default is 1 (=> start of
--  the table)
-- @see gears.table.iterate_value
function gtable.iterate(t, filter, start)
    local count  = 0
    local index  = start or 1
    local length = #t

    return function ()
        while count < length do
            local item = t[index]
            index = gmath.cycle(#t, index + 1, true)
            count = count + 1
            if filter(item) then return item end
        end
    end
end

--- Get the next (or previous) value from a table and cycle if necessary.
--
-- If the table contains the same value multiple type (aka, is not a set), the
-- `first_index` has to be specified.
--
-- @tparam table t The input table.
-- @param value A value from the table.
-- @tparam[opt=1] number step_size How many element forward (or backward) to pick.
-- @tparam[opt=nil] function filter An optional function. When it returns
--  `false`, the element are skipped until a match if found. It takes the value
--  as its sole parameter.
-- @tparam[opt=1] number start_at Where to start the lookup from.
-- @return The value. If there is a filter function and no element match,
--  then `nil` is returned.
-- @treturn number|nil The element (if any) key.
function gtable.iterate_value(t, value, step_size, filter, start_at)
    local k = gtable.hasitem(t, value, true, start_at)
    if not k then return end

    step_size = step_size or 1
    local new_key = gmath.cycle(#t, k + step_size)

    if filter and not filter(t[new_key]) then
        for i=1, #t do
            local k2 = gmath.cycle(#t, new_key + i)
            if filter(t[k2]) then
                return t[k2], k2
            end
        end
        return
    end

    return t[new_key], new_key
end

--- Merge items from the one table to another one
-- @class function
-- @name merge
-- @tparam table t the container table
-- @tparam table set the mixin table
-- @treturn table Return `t` for convenience
function gtable.merge(t, set)
    for _, v in ipairs(set) do
        table.insert(t, v)
    end
    return t
end

--- Map a function to a table. The function is applied to each value
-- on the table, returning a table of the results.
-- @class function
-- @name map
-- @tparam function f the function to be applied to each value on the table
-- @tparam table tbl the container table whose values will be operated on
-- @treturn table Return a table of the results
function gtable.map(f, tbl)
    local t = {}
    for k,v in pairs(tbl) do
        t[k] = f(v)
    end
    return t
end


return gtable
