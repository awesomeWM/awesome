---------------------------------------------------------------------------
--- An helper module to map userdata __index and __newindex entries to
-- lua classes.
--
-- @author Emmanuel Lepage-Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage-Vallee
-- @submodule gears.object
---------------------------------------------------------------------------

local gtable = require("gears.table")
local gtimer = nil --require("gears.timer")
-- local gdebug = require("gears.debug")
local object = {}
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)


--- Add the missing properties handler to a CAPI object such as client/tag/screen.
-- Valid args:
--
-- * **getter**: A smart getter (handle property getter itself)
-- * **getter_fallback**: A dumb getter method (don't handle individual property getter)
-- * **getter_class**: A module with individual property getter/setter
-- * **getter_prefix**: A special getter prefix (like "get" or "get_" (default))
-- * **setter**: A smart setter (handle property setter itself)
-- * **setter_fallback**: A dumb setter method (don't handle individual property setter)
-- * **setter_class**: A module with individual property getter/setter
-- * **setter_prefix**: A special setter prefix (like "set" or "set_" (default))
-- * **auto_emit**: Emit "property::___" automatically (default: false). This is
--     ignored when setter_fallback is set or a setter is found
--
-- @param class A standard luaobject derived object
-- @tparam[opt={}] table args A set of accessors configuration parameters
-- @function gears.object.properties.capi_index_fallback
function object.capi_index_fallback(class, args)
    args = args or {}

    local getter_prefix = args.getter_prefix or "get_"
    local setter_prefix = args.setter_prefix or "set_"

    local getter = args.getter or function(cobj, prop)
        -- Look for a getter method
        if args.getter_class and args.getter_class[getter_prefix..prop] then
            return args.getter_class[getter_prefix..prop](cobj)
        elseif args.getter_class and args.getter_class["is_"..prop] then
            return args.getter_class["is_"..prop](cobj)
        end

        -- Make sure something like c:a_mutator() works
        if args.getter_class and args.getter_class[prop] then
            return args.getter_class[prop]
        end
        -- In case there is already a "dumb" getter like `awful.tag.getproperty'
        if args.getter_fallback then
            return args.getter_fallback(cobj, prop)
        end

        -- Use the fallback property table
        assert(prop ~= "_private")
        return cobj._private[prop]
    end

    local setter = args.setter or function(cobj, prop, value)
        -- Look for a setter method
        if args.setter_class and args.setter_class[setter_prefix..prop] then
            return args.setter_class[setter_prefix..prop](cobj, value)
        end

        -- In case there is already a "dumb" setter like `awful.client.property.set'
        if args.setter_fallback then
            return args.setter_fallback(cobj, prop, value)
        end

        -- If a getter exists but not a setter, then the property is read-only
        if args.getter_class and args.getter_class[getter_prefix..prop] then
            return
        end

        -- Use the fallback property table
        cobj._private[prop] = value

        -- Emit the signal
        if args.auto_emit then
            cobj:emit_signal("property::"..prop, value)
        end
    end

    assert(type(class) ~= "function")

    -- Attach the accessor methods
    class.set_index_miss_handler(getter)
    class.set_newindex_miss_handler(setter)
end

-- Convert the capi objects back into awful ones.
local function deprecated_to_current(content)
    local first   = content[1]

    local current = first and first._private and
        first._private._legacy_convert_to or nil

    if not current then return nil end

    local ret = {current}

    for _, o in ipairs(content) do
        -- If this is false, someone tried to mix things in a
        -- way that is definitely not intentional.
        assert(o._private and o._private._legacy_convert_to)

        if o._private._legacy_convert_to ~= current then
            -- If this is false, someone tried to mix things in a
            -- way that is definitely not intentional.
            assert(o._private._legacy_convert_to)

            table.insert(
                ret, o._private._legacy_convert_to
            )
            current = o._private._legacy_convert_to
        end
    end

    return ret
end

-- (private api)
-- Many legacy Awesome APIs such as `client:tags()`, `root.buttons()`,
-- `client:keys()`, `drawin:geometry()`, etc used functions for both the getter
-- and setter. This contrast with just about everything else that came after
-- it and is an artifact of an earlier time before we had "good" Lua object
-- support.
--
-- Because both consistency and backward compatibility are important, this
-- table wrapper allows to support both the legacy method based accessors
-- and key/value based accessors.
--
-- TO BE USED FOR DEPRECATION ONLY.

local function copy_object(obj, to_set, name, capi_name, is_object, join_if, set_empty)
    local ret = gtable.clone(to_set, false)

    -- .buttons used to be a function taking the result of `gears.table.join`.
    -- For compatibility, support this, but from now on, it's a property.
    return setmetatable(ret, {
        __call = function(_, self, new_objs)
--TODO uncomment
--             gdebug.deprecate("`"..name.."` is no longer a function, it is a property. "..
--                 "Remove the `gears.table.join` and use a brace enclosed table",
--                 {deprecated_in=5}
--             )

            new_objs, self = is_object and new_objs or self, is_object and self or obj

            -- Setter
            if new_objs and #new_objs > 0 then
                if (not set_empty) and not next(new_objs) then return end

                local is_formatted = join_if(new_objs)

                -- Because modules may rely on :buttons taking a list of
                -- `capi.buttons`/`capi.key` and now the user passes a list of
                -- `awful.button`/`awful.key`  convert this.

                local result = is_formatted and
                    new_objs or gtable.join(unpack(new_objs))

                -- First, when possible, get rid of the legacy format and go
                -- back to the non-deprecated code path.
                local current = self["set_"..name]
                    and deprecated_to_current(result) or nil

                if current then
                    self["set_"..name](self, current)
                    return capi_name and self[capi_name](self) or self[name]
                elseif capi_name and is_object then
                    return self[capi_name](self, result)
                elseif capi_name then
                    return self[capi_name](result)
                else
                    self._private[name.."_formatted"] = result
                    return self._private[name.."_formatted"]
                end
            end

            -- Getter
            if capi_name and is_object then
                return self[capi_name](self)
            elseif capi_name then
                return self[capi_name]()
            else
                return self._private[name.."_formatted"] or {}
            end
        end
    })
end

function object._legacy_accessors(obj, name, capi_name, is_object, join_if, set_empty, delay, add_append_name)
    delay = delay or add_append_name

    -- Some objects have a special "object" property to add more properties, but
    -- not all.

    local magic_obj = obj.object and obj.object or obj

    magic_obj["get_"..name] = function(self)
        self = is_object and self or obj

        self._private[name] = self._private[name] or copy_object(
            obj, {}, name, capi_name, is_object, join_if, set_empty
        )

        local current = deprecated_to_current(self._private[name])

        return current or self._private[name]
    end

    magic_obj["set_"..name] = function(self, objs)
        if (not set_empty) and not next(objs) then return end

        if not is_object then
            objs, self = self, obj
        end

        -- When using lua expressions like `false and true and my_objects`,
        -- it is possible the code ends up as a boolean rather than `nil`
        -- the resulting type is sometime counter intuitive and different
        -- from similar languages such as JavaScript. Be forgiving and correct
        -- course.
        if objs == false then
            objs = nil
        end

        -- Sometime, setting `nil` might be volontary since the user might
        -- expect it will act as "clear". The correct thing would be to set
        -- `{}`, but allow it nevertheless.

        if objs == nil then
            objs = {}
        end

        assert(self)

        -- When called from a declarative property list, "buttons" will be set
        -- using the result of gears.table.join, detect this
        local is_formatted = join_if(objs)

--         if is_formatted then
--TODO uncomment
--             gdebug.deprecate("Remove the `gears.table.join` and replace it with braces",
--                 {deprecated_in=5}
--             )
--         end


        --TODO v6 Use the original directly and drop this legacy copy
        local function apply()
            local result = is_formatted and objs
                or gtable.join(unpack(objs))

            if is_object and capi_name then
                self[capi_name](self, result)
            elseif capi_name then
                obj[capi_name](result)
            else
                self._private[name.."_formatted"] = result
            end
        end

        -- Some properties, like keys, are expensive to set, schedule them
        -- instead.
        if not delay then
            apply()
        else
            if not self._private["_delayed_"..name] then
                gtimer = gtimer or require("gears.timer")
                gtimer.delayed_call(function()
                    self._private["_delayed_"..name]()
                    self._private["_delayed_"..name] = nil
                end)
            end

            self._private["_delayed_"..name] = apply
        end

        self._private[name] = copy_object(
            obj, objs, name, capi_name, is_object, join_if, set_empty
        )
    end

    if add_append_name then
        magic_obj["append_"..add_append_name] = function(self, obj2)
            self._private[name] = self._private[name]
                or magic_obj["get_"..name](self, nil)

            table.insert(self._private[name], obj2)

            magic_obj["set_"..name](self, self._private[name])
        end

        magic_obj["remove_"..add_append_name] = function(self, obj2)
            self._private[name] = self._private[name]
                or magic_obj["get_"..name](self, nil)

            for k, v in ipairs(self._private[name]) do
                if v == obj2 then
                    table.remove(self._private[name], k)
                    break
                end
            end

            magic_obj["set_"..name](self, self._private[name])
        end
    end
end

return setmetatable( object, {__call = function(_,...) object.capi_index_fallback(...) end})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
