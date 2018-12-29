---------------------------------------------------------------------------
-- @author Emmanuel Lepage-Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2018-2019 Emmanuel Lepage-Vallee
-- @module root
---------------------------------------------------------------------------

local capi = { root = root }
local gtable = require("gears.table")
local gtimer = require("gears.timer")
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

for _, type_name in ipairs { "button", "key" } do
    local prop_name = type_name.."s"

    -- The largest amount of wall clock time when loading Awesome 3.4 rc.lua
    -- was the awful.util.table.join (now gears.table.join). While the main
    -- bottleneck in the newer releases moved into LGI, doing all these `join`
    -- slow startup down quite a lot. On top of that, with the ability to add
    -- and remove keys and buttons can cause a large overhead of its own. To
    -- mitigate that, only set the actual content once per main loop iteration.
    --
    -- The C code also delay uploading these keys into the X server to prevent
    -- too many keyboard map changes from freezing Awesome.
    local has_delayed, added, removed = false, {}, {}

    local function delay(value)
        if value then
            table.insert(added, value)
        end

        if has_delayed then return end

        has_delayed = true

        gtimer.delayed_call(function()
            local new_values = capi.root["_"..prop_name]()

            -- In theory, because they are inserted ordered, it is safe to assume
            -- the once found, the capi.key/button will be next to each other.
            for _, v in ipairs(removed) do
                local idx = gtable.hasitem(new_values, v[1])

                if idx then
                    for i=1, #v do
                        assert(
                            new_values[idx] == v[i],
                            "The root private "..type_name.." table is corrupted"
                        )

                        table.remove(new_values, idx)
                    end
                end

                idx = gtable.hasitem(added, v)

                if idx then
                    table.remove(added, idx)
                end
            end

            local joined = gtable.join(unpack(added))
            new_values = gtable.merge(new_values, joined)

            capi.root["_"..prop_name](new_values)

            has_delayed, added, removed = false, {}, {}
        end)
    end

    capi.root["_append_"..type_name] = function(value)
        if not value then return end

        local t1 = capi.root._private[prop_name]

        -- Simple case
        if (not t1) or not next(t1) then
            capi.root[prop_name] = {value}
            assert(capi.root._private[prop_name])
            return
        end

        delay(value)
    end

    capi.root["_append_"..prop_name] = function(values)
        -- It's pointless to use gears.table.merge, in the background it has the
        -- same loop anyway. Also, this isn't done very often.
        for _, value in ipairs(values) do
            capi.root["_append_"..type_name](value)
        end
    end

    capi.root["_remove_"..type_name] = function(value)
        if not capi.root._private[prop_name] then return end

        local k = gtable.hasitem(capi.root._private[prop_name], value)

        if k then
            table.remove(capi.root._private[prop_name], k)
        end

        -- Because of the legacy API, it is possible the capi.key/buttons will
        -- be in the formatted table but not of the awful.key/button one.
        assert(value[1])

        table.insert(removed, value)
    end

    capi.root["has_"..type_name] = function(item)
        if not item["_is_capi_"..type_name] then
            item = item[1]
        end

        return gtable.hasitem(capi.root["_"..prop_name](), item) ~= nil
    end

    assert(root[prop_name])

end
