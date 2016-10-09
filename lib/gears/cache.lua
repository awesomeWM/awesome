---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
-- @classmod gears.cache
---------------------------------------------------------------------------

local select = select
local setmetatable = setmetatable
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local cache = {}

--- Get an entry from the cache, creating it if it's missing.
-- @param ... Arguments for the creation callback. These are checked against the
--   cache contents for equality.
-- @return The entry from the cache
function cache:get(...)
    local result = self._cache
    for i = 1, select("#", ...) do
        local arg = select(i, ...)
        local next = result[arg]
        if not next then
            next = {}
            result[arg] = next
        end
        result = next
    end
    local ret = result._entry
    if not ret then
        ret = { self._creation_cb(...) }
        result._entry = ret
    end
    return unpack(ret)
end

--- Create a new cache object. A cache keeps some data that can be
-- garbage-collected at any time, but might be useful to keep.
-- @param creation_cb Callback that is used for creating missing cache entries.
-- @return A new cache object.
function cache.new(creation_cb)
    return setmetatable({
        _cache = setmetatable({}, { __mode = "v" }),
        _creation_cb = creation_cb
    }, {
        __index = cache
    })
end

return setmetatable(cache, { __call = function(_, ...) return cache.new(...) end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
