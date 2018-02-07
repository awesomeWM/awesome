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
-- @class function
-- @name hasitem
-- @param t The table.
-- @param item The item to look for in values of the table.
-- @return The key were the item is found, or nil if not found.
function gtable.hasitem(t, item)
    for k, v in pairs(t) do
        if v == item then
            return k
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

---
-- Returns an iterator to cycle through, starting from the first element or the
-- given index, all elements of a table that match a given criteria.
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

--- Slice a table.
--
-- @tparam table t The table to slice.
-- @tparam[opt=1] int start The start index.
-- @tparam[opt=last entry] int end_ The end index.
-- @tparam[opt=1] int step The stepping.
-- @treturn table
function gtable.slice(t, start, end_, step)
  local sliced = {}
  for i = start or 1, end_ or #t, step or 1 do
    sliced[#sliced+1] = t[i]
  end
  return sliced
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
