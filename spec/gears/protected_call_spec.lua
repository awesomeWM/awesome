---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2016 Uli Schlachter
---------------------------------------------------------------------------

local gdebug = require("gears.debug")
local protected_call = require("gears.protected_call")

describe("gears.protected_call", function()
    -- Stop the error reporting during tests
    local orig_print_error = gdebug.print_error
    local errors
    before_each(function()
        errors = {}
        gdebug.print_error = function(err)
            table.insert(errors, err)
        end
    end)
    after_each(function()
        gdebug.print_error = orig_print_error
    end)

    it("Call with arguments and result", function()
        local called = false
        local function f(...)
            called = true
            assert.is_same({ ... }, { 1, "second" })
            return "first", 2
        end
        local results = { protected_call(f, 1, "second") }
        assert.is_true(called)
        assert.is_same({ "first", 2 }, results)
        assert.is_same(errors, {})
    end)

    it("Call with error", function()
        assert.is_same({}, { protected_call(error, "I was called") })
        assert.is_same(#errors, 1)
        assert.is_truthy(string.find(errors[1], "I was called"))
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
