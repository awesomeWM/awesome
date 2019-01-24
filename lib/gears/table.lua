---------------------------------------------------------------------------
--- Table module for gears
--
-- @module gears.table
---------------------------------------------------------------------------


local rtable = table

local gmath = require("gears.math")
local gtable = {}

--- Join all tables given as arguments.
-- This will iterate over all tables and insert their entries into a new table.
-- @class function
-- @name join
-- @tparam table ... Tables to join.
-- @treturn table A new table containing all entries from the arguments.
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
-- @tparam[opt=false] bool raw Use rawset (avoid the metatable)
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

--- Pack all elements with an integer key into a new table.
-- While both lua and luajit implement __len over sparse
-- tables, the standard defines it as an implementation
-- detail.
--
-- This function removes any entries with non-numeric keys.
--
-- @class function
-- @name from_sparse
-- @tparam table t A potentially sparse table.
-- @treturn table A packed table with only numeric keys.
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
-- @class function
-- @name hasitem
-- @tparam table t The table.
-- @param item The item to look for in values of the table.
-- @treturn[1] string|number The key of the item.
-- @treturn[2] nil
function gtable.hasitem(t, item)
    for k, v in pairs(t) do
        if v == item then
            return k
        end
    end
end

--- Get all matching table keys for a `matcher` function.
--
-- @tparam table t The table.
-- @tparam function matcher A function taking the key and value as arguments and
--   returning a boolean.
-- @tparam[opt=false] boolean ordered If true, only look for continuous
--   numeric keys.
-- @tparam[opt=nil] number max The maximum number of entries to find.
-- @treturn table|nil An ordered table with all the keys or `nil` if none were
--   found.
function gtable.find_keys(t, matcher, ordered, max)
    if max == 0 then return nil end

    ordered, max = ordered or false, 0
    local ret, it = {}, ordered and ipairs or pairs

    for k, v in it(t) do
        if matcher(k,v) then
            table.insert(ret, k)

            if #ret == max then break end
        end
    end

    return #ret > 0 and ret or nil
end

--- Find the first key that matches a function.
--
-- @tparam table t The table.
-- @tparam function matcher A function taking the key and value as arguments and
--   returning a boolean.
-- @tparam[opt=false] boolean ordered If true, only look for continuous
--   numeric keys.
-- @return The table key or nil
function gtable.find_first_key(t, matcher, ordered)
    local ret = gtable.find_keys(t, matcher, ordered, 1)

    return ret and ret[1] or nil
end


--- Get a sorted table with all integer keys from a table.
-- @class function
-- @name keys
-- @tparam table t The table for which the keys to get.
-- @treturn table A table with keys
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

--- Filter a table's keys for certain content type.
-- @class function
-- @name keys_filter
-- @tparam table t The table to retrieve the keys for.
-- @tparam string ... The types to look for.
-- @treturn table A filtered table.
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

--- Reverse a table.
-- @class function
-- @name reverse
-- @tparam table t The table to reverse.
-- @treturn table A reversed table.
function gtable.reverse(t)
    local tr = { }
    -- Reverse all elements with integer keys.
    for _, v in ipairs(t) do
        rtable.insert(tr, 1, v)
    end
    -- Add the remaining elements.
    for k, v in pairs(t) do
        if type(k) ~= "number" then
            tr[k] = v
        end
    end
    return tr
end

--- Clone a table.
-- @class function
-- @name clone
-- @tparam table t The table to clone.
-- @tparam[opt=true] bool deep Create a deep clone?
-- @treturn table A clone of `t`.
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

--- Iterate over a table.
-- Returns an iterator to cycle through all elements of a table that match a
-- given criteria, starting from the first element or the given index.
--
-- @class function
-- @name iterate
-- @tparam table t The table to iterate.
-- @tparam func  filter A function that returns true to indicate a positive
--   match.
-- @param func.item The item to filter.
-- @tparam[opt=1] int start Index to start iterating from.
--   Default is 1 (=> start of the table).
-- @treturn func
function gtable.iterate(t, filter, start)
    local count  = 0
    local index  = start or 1
    local length = #t

    return function ()
        while count < length do
            local item = t[index]
            index = gmath.cycle(#t, index + 1)
            count = count + 1
            if filter(item) then return item end
        end
    end
end

--- Merge items from one table to another one.
-- @class function
-- @name merge
-- @tparam table t The container table
-- @tparam table set The mixin table.
-- @treturn table (for convenience)
function gtable.merge(t, set)
    for _, v in ipairs(set) do
        table.insert(t, v)
    end
    return t
end

--- Map a function to a table.
-- The function is applied to each value on the table, returning a modified
-- table.
-- @class function
-- @name map
-- @tparam function f The function to be applied to each value in the table.
-- @tparam table tbl The container table whose values will be operated on.
-- @treturn table
function gtable.map(f, tbl)
    local t = {}
    for k,v in pairs(tbl) do
        t[k] = f(v)
    end
    return t
end

return gtable
