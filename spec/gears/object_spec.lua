---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local object = require("gears.object")

describe("gears.object", function()
    local obj
    before_each(function()
        obj = object()
    end)

    it("strong connecting and emitting signal", function()
        local called = false
        local function cb()
            called = true
        end
        obj:connect_signal("signal", cb)
        obj:emit_signal("signal")
        assert.is_true(called)
    end)

    it("weak connecting and emitting signal", function()
        local called = false
        local function cb()
            called = true
        end
        obj:weak_connect_signal("signal", cb)

        -- Check that the GC doesn't disconnect the signal
        for _ = 1, 10 do
            collectgarbage("collect")
        end

        obj:emit_signal("signal")
        assert.is_true(called)
    end)

    it("strong connecting, disconnecting and emitting signal", function()
        local called = false
        local function cb()
            called = true
        end
        obj:connect_signal("signal", cb)
        obj:disconnect_signal("signal", cb)
        obj:emit_signal("signal")
        assert.is_false(called)
    end)

    it("weak connecting, disconnecting and emitting signal", function()
        local called = false
        local function cb()
            called = true
        end
        obj:weak_connect_signal("signal", cb)
        obj:disconnect_signal("signal", cb)
        obj:emit_signal("signal")
        assert.is_false(called)
    end)

    it("arguments to signal", function()
        obj:connect_signal("signal", function(arg1, arg2)
            assert.is.equal(obj, arg1)
            assert.is.same(42, arg2)
        end)
        obj:weak_connect_signal("signal", function(arg1, arg2)
            assert.is.equal(obj, arg1)
            assert.is.same(42, arg2)
        end)
        obj:emit_signal("signal", 42)
    end)

    it("strong non-auto disconnect", function()
        local called = false
        obj:connect_signal("signal", function()
            called = true
        end)
        collectgarbage("collect")
        obj:emit_signal("signal")
        assert.is_true(called)
    end)

    it("weak auto disconnect", function()
        local called = false
        obj:weak_connect_signal("signal", function()
            called = true
        end)
        collectgarbage("collect")
        obj:emit_signal("signal")
        assert.is_false(called)
    end)

    it("strong connect after weak connect", function()
        local function cb() end
        obj:weak_connect_signal("signal", cb)
        assert.has.errors(function()
            obj:connect_signal("signal", cb)
        end)
    end)

    it("weak connect after strong connect", function()
        local function cb() end
        obj:connect_signal("signal", cb)
        assert.has.errors(function()
            obj:weak_connect_signal("signal", cb)
        end)
    end)

    it("weak signal is disconnected before __gc runs", function()
        local finalized = false
        do
            local userdata
            local function callback()
                error("Signal should already be disconnected")

                -- This should reference the object...
                print("userdata:", userdata)
            end
            obj:weak_connect_signal("signal", callback)

            -- Now create an object and tie its lifetime to the callback
            local function gc()
                finalized = true
            end
            if _VERSION <= "Lua 5.1" then
                -- luacheck: globals newproxy
                userdata = newproxy(true)
                getmetatable(userdata).__gc = gc
                getmetatable(userdata).callback = callback
            else
                userdata = setmetatable({callback}, { __gc = gc })
            end
        end
        collectgarbage("collect")
        assert.is_true(finalized)
        obj:emit_signal("signal")
    end)

    it("dynamic property disabled", function()
        local class = {}
        function class:get_foo() return "bar" end

        local obj2 = object{class=class}

        obj2.foo = 42

        assert.is.equal(obj2.foo, 42)
    end)

    it("dynamic property disabled", function()
        local class = {}
        function class:get_foo() return "bar" end

        local obj2 = object{class=class, enable_properties = true}

        assert.has_error(function()
            obj2.foo = 42
        end)

        assert.is.equal(obj2.foo, "bar")
    end)

    it("auto emit disabled", function()
        local got_it = false
        obj:connect_signal("property::foo", function() got_it=true end)

        obj.foo = 42

        assert.is_false(got_it)
    end)

    it("auto emit enabled", function()
        local got_it = false
        local obj2 = object{enable_auto_signals=true, enable_properties=true}
        obj2:connect_signal("property::foo", function() got_it=true end)

        obj2.foo = 42

        assert.is_true(got_it)
    end)

    it("auto emit without dynamic properties", function()
        assert.has.errors(function()
            object{enable_auto_signals=true, enable_properties=false}
        end)
    end)

    it("is_signal_connected", function()
        local cb = function()end
        assert.is_false(obj:is_signal_connected("signal", cb))
        obj:connect_signal("signal", cb)
        assert.is_true(obj:is_signal_connected("signal", cb))
    end)

    it("is_weak_signal_connected", function()
        local cb = function()end
        assert.is_false(obj:is_signal_connected("signal", cb))
        obj:weak_connect_signal("signal", cb)
        assert.is_true(obj:is_signal_connected("signal", cb))
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
