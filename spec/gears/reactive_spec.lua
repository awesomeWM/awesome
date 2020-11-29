---------------------------------------------------------------------------
-- @author Emmanuel Lepage-Vallee
-- @copyright 2020 Emmanuel Lepage-Vallee &lt;elv1313@gmail.com&gt;
---------------------------------------------------------------------------
_G.awesome.connect_signal = function() end

local reactive = require("gears.reactive")
local gobject = require("gears.object")

-- Keep track of the number of time the value changed.
local change_counter, last_counter = 0, 0

local function has_changed()
    local ret = change_counter > last_counter
    last_counter = change_counter
    return ret
end

describe("gears.reactive", function()
    -- Unsupported.
    if not debug.upvaluejoin then return end -- luacheck: globals debug.upvaluejoin

    local myobject1 = gobject {
        enable_properties   = true,
        enable_auto_signals = true
    }

    local myobject2 = gobject {
        enable_properties   = true,
        enable_auto_signals = true
    }

    local myobject3 = gobject {
        enable_properties   = true,
        enable_auto_signals = true
    }

    -- This will create a property with a signal, we will need that later.
    myobject3.bar = "baz"
    myobject1.foo = 0

    -- Using rawset wont add a signal. It means the change isn't visible to the
    -- `gears.reactive` expression. However, we still want to make sure it can
    -- use the raw property even without change detection.
    rawset(myobject2, "obj3", myobject3)

    -- Use a string to compare the address. We can't use `==` since
    -- `gears.reactive` re-implement it to emulate the `==` of the source
    -- objects.
    local hash, hash2, hash3 = tostring(myobject1), tostring(myobject2), tostring(print)

    -- Make sure the proxy wrapper isn't passed to the called functions.
    local function check_no_proxy(obj)
        assert.is.equal(rawget(obj, "_reactive"), nil)
        assert.is.equal(hash, tostring(obj))
    end

    -- With args.
    function myobject1:method1(a, b, obj)
        -- Make sure the proxy isn't propagated.
        assert.is.equal(hash, tostring(obj))
        assert.is.equal(hash, tostring(self))
        assert.is.falsy(obj._reactive)
        assert.is.falsy(self._reactive)

        -- Check the arguments.
        assert.is.equal(a, 1)
        assert.is.equal(b, 2)

        return myobject2, 42
    end

    -- With no args.
    function myobject1:method2(a)
        assert(a == nil)
        assert(not self._reactive)
        assert(hash == tostring(self))
    end

    -- Create some _ENV variables. `gears.reactive` cannot detect the changes,
    -- at least for now. This is to test if they can be used regardless.
    local i, r = 1337, nil

    it("basic creation", function()
        r = reactive(function()
            -- Skip busted, it uses its own debug magic which collide with
            -- gears.reactive sandboxes.
            local assert, tostring = rawget(_G, "assert"), rawget(_G, "tostring")

            -- Using _G directly should bypass the proxy. It least until more
            -- magic is implemented to stop it. So better test it too.
            local realprint = _G.print
            assert(tostring(realprint) == hash3)

            -- But the "local" one should be proxy-ed to prevent the internal
            -- proxy objects from leaking when calling a function outside of the
            -- sandbox.
            assert(tostring(print) == hash3)

            -- Make sure we got a proxy.
            assert(myobject1._reactive)

            assert(not myobject1:method2())

            local newobject, other = myobject1:method1(1,2, myobject1)

            -- Make sure the returned objects are proxied properly.
            assert(type(other) == "number")
            assert(other == 42)
            assert(newobject._reactive)
            assert(tostring(newobject) == tostring(myobject2))
            assert(tostring(newobject) == hash2)

            -- Now call an upvalue local function
            check_no_proxy(myobject1)

            return {
                not_object         = i,
                object_expression  = (myobject1.foo + 42),
                nested_object_tree = myobject2.obj3.bar,
                original_obj       = myobject1
            }
        end)

        r:connect_signal("property::value", function()
            change_counter = change_counter + 1
        end)

        assert.is_false(has_changed())

        -- Make sure that the reactive proxy didn't override the original value.
        -- And yes, it's actually possible and there is explicit code to avoid
        -- it.
        assert.is.equal(hash, tostring(myobject1))
    end)

    it("basic_changes", function()
        local val = r.value

        -- The delayed magic should be transparent. It will never work
        -- in the unit test, but it should not cause any visible behavior
        -- change. It would not be magic if it was.
        assert(val)

        -- Disable delayed.
        r._private.value = nil
        r._private.evaluated = false
        assert.is_true(r.delayed)
        r.delayed = false
        assert.is.falsy(r.delayed)

        val = r.value
        assert(val)

        -- Make sure the proxy didn't leak into the return value
        assert.is.falsy(rawget(val, "_reactive"))
        assert.is.falsy(rawget(val.original_obj, "_reactive"))

        assert.is_true(has_changed())

        assert.is.equal(r._private.value.object_expression, 42)
        assert.is.equal(r._private.value.not_object, 1337)

        myobject1.foo = 1

        assert.is_true(has_changed())

        assert.is.equal(r._private.value.object_expression, 43)

        -- Known limitation.
        i = 1338
        assert.is.equal(r._private.value.not_object, 1337)
        r:refresh()
        assert.is.equal(r._private.value.not_object, 1338)

        -- Ensure that nested (and raw-setted) object property changes
        -- are detected.
        assert.is.equal(r._private.value.nested_object_tree, "baz")
        myobject3.bar = "bazz"
        assert.is_true(has_changed())
        assert.is.equal(r._private.value.nested_object_tree, "bazz")
    end)

    -- gears.reactive play with the metatable operators a lot.
    -- Make sure one of them work.
    it("test tostring", function()
        local myobject4 = gobject {
            enable_properties   = true,
            enable_auto_signals = true
        }

        local mt = getmetatable(myobject4)

        mt.__tostring = function() return "lol" end

        local react = reactive(function()
            _G.assert(myobject4._reactive)
            _G.assert(tostring(myobject4) == "lol")

            return tostring(myobject4)
        end)

        local val = react.value

        assert.is.equal(val, "lol")
    end)

    it("test disconnect", function()
        r:disconnect()
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
