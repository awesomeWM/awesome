---------------------------------------------------------------------------
-- @author Lucas Schwiderski
-- @copyright 2022 Lucas Schwiderski
---------------------------------------------------------------------------

local conditional = require("wibox.container.conditional")
local utils = require("wibox.test_utils")
local base = require("wibox.widget.base")
local p = require("wibox.widget.base").place_widget_at

describe("wibox.container.conditional", function()
    it("implements the common API", function()
        utils.test_container(conditional())
    end)

    describe("constructor", function()
        it("applies arguments", function()
            local inner = base.empty_widget()
            local state = false
            local widget = conditional(inner, state)

            assert.is.equal(inner, widget.widget)
            assert.is.equal(state, widget.state)
        end)

        it("defaults to state == true", function()
            local widget = conditional()
            assert.is_true(widget.state)
        end)
    end)

    it("hides the child widget when state == false", function()
        local inner = utils.widget_stub(10, 10)
        local widget = conditional(inner)

        assert.widget_fit(widget, { 10, 10 }, { 10, 10 })
        assert.widget_layout(widget, { 10, 10 }, {
            p(inner,  0,  0, 10, 10),
        })

        widget.state = false

        assert.widget_fit(widget, { 10, 10 }, { 0, 0 })
        assert.widget_layout(widget, { 0, 0 }, {})
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
