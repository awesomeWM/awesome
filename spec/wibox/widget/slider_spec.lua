---------------------------------------------------------------------------
-- @author Aire-One
-- @copyright 2018 Aire-One
---------------------------------------------------------------------------

local slider = require("wibox.widget.slider")

describe("wibox.widget.slider", function()
    local widget = nil

    before_each(function()
        widget = slider()
    end)

    describe("constructor", function()
        it("applies arguments", function()
            local widget = slider({ value = 10 })

            assert.is.equal(widget.value, 10)
        end)
    end)

    describe("signal property::value", function()
        local property_value = nil
        before_each(function()
            widget:connect_signal("property::value", function()
                property_value = property_value + 1
            end)

            property_value = 0
        end)

        it("signal property::value", function()
            -- initial stats
            assert.is.equal(property_value, 0)
            assert.is.equal(widget.value, 0)

            -- test to direct change the value
            -- it should call the automated "property changed" signal
            widget.value = 50
            assert.is.equal(property_value, 1)
            assert.is.equal(widget.value, 50)

            -- test with the setter
            widget:set_value(100)
            assert.is.equal(property_value, 2)
            assert.is.equal(widget.value, 100)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
