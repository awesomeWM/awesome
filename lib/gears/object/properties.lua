---------------------------------------------------------------------------
--- An helper module to map userdata __index and __newindex entries to
-- lua classes.
--
-- @author Emmanuel Lepage-Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage-Vallee
-- @submodule gears.object
---------------------------------------------------------------------------

local object = {}


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

return setmetatable( object, {__call = function(_,...) object.capi_index_fallback(...) end})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
