---------------------------------------------------------------------------
--- An helper module to map userdata __index and __newindex entries to
-- lua classes.
--
-- @author Emmanuel Lepage-Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage-Vallee
-- @submodule gears.object
---------------------------------------------------------------------------

local gtable = require("gears.table")
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
        return cobj.data[prop]
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
        cobj.data[prop] = value

        -- Emit the signal
        if args.auto_emit then
            cobj:emit_signal("property::"..prop, value)
        end
    end

    -- Attach the accessor methods
    class.set_index_miss_handler(getter)
    class.set_newindex_miss_handler(setter)
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

                if capi_name and is_object then
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

function object._legacy_accessors(obj, name, capi_name, is_object, join_if, set_empty)

    -- Some objects have a special "object" property to add more properties, but
    -- not all.

    local magic_obj = obj.object and obj.object or obj

    magic_obj["get_"..name] = function(self)
        self = is_object and self or obj

        --FIXME v5 all objects should use _private instead of getproperty.
        if not self._private then
            self._private = {}
        end

        self._private[name] = self._private[name] or copy_object(
            obj, {}, name, capi_name, is_object, join_if, set_empty
        )


        assert(self._private[name])
        return self._private[name]
    end

    magic_obj["set_"..name] = function(self, objs)
        if (not set_empty) and not next(objs) then return end

        if not is_object then
            objs, self = self, obj
        end
        assert(objs)

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
        local result = is_formatted and objs
            or gtable.join(unpack(objs))

        if is_object and capi_name then
            self[capi_name](self, result)
        elseif capi_name then
            obj[capi_name](result)
        else
            self._private[name.."_formatted"] = result
        end

        self._private[name] = copy_object(
            obj, objs, name, capi_name, is_object, join_if, set_empty
        )
    end
end

return setmetatable( object, {__call = function(_,...) object.capi_index_fallback(...) end})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
