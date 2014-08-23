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

    it("connect non-existent signal", function()
        assert.has.errors(function()
            obj:connect_signal("foo", function() end)
        end)
    end)

    it("disconnect non-existent signal", function()
        assert.has.errors(function()
            obj:disconnect_signal("foo", function() end)
        end)
    end)

    it("emitting non-existent signal", function()
        assert.has.errors(function()
            obj:emit_signal("foo")
        end)
    end)

    it("connecting and emitting signal", function()
        local called = false
        obj:connect_signal("signal", function()
            called = true
        end)
        obj:emit_signal("signal")
        assert.is_true(called)
    end)

    it("connecting, disconnecting and emitting signal", function()
        local called = false
        local function cb()
            called = true
        end
        obj:connect_signal("signal", cb)
        obj:disconnect_signal("signal", cb)
        obj:emit_signal("signal")
        assert.is_false(called)
    end)

    it("arguments to signal", function()
        obj:connect_signal("signal", function(arg1, arg2)
            assert.is.equal(obj, arg1)
            assert.is.same(42, arg2)
        end)
        obj:emit_signal("signal", 42)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
