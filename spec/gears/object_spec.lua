---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local object = require("gears.object")

describe("gears.object", function()
    local obj
    before_each(function()
        obj = object()
        obj:add_signal("signal")
    end)

    it("strong connect non-existent signal", function()
        assert.has.errors(function()
            obj:connect_signal("foo", function() end)
        end)
    end)

    it("weak connect non-existent signal", function()
        assert.has.errors(function()
            obj:weak_connect_signal("foo", function() end)
        end)
    end)

    it("strong disconnect non-existent signal", function()
        assert.has.errors(function()
            obj:disconnect_signal("foo", function() end)
        end)
    end)

    it("emitting non-existent signal", function()
        assert.has.errors(function()
            obj:emit_signal("foo")
        end)
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
        for i = 1, 10 do
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
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
