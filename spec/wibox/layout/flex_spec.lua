---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local flex = require("wibox.layout.flex")
local utils = require("wibox.test_utils")
local p = require("wibox.widget.base").place_widget_at

describe("wibox.layout.flex", function()
    local layout
    before_each(function()
        layout = flex.vertical()
    end)

    it("empty layout fit", function()
        assert.widget_fit(layout, { 10, 10 }, { 0, 0 })
    end)

    it("empty layout layout", function()
        assert.widget_layout(layout, { 0, 0 }, {})
    end)

    describe("with widgets", function()
        local first, second, third

        before_each(function()
            first = utils.widget_stub(10, 10)
            second = utils.widget_stub(15, 15)
            third = utils.widget_stub(10, 10)

            layout:add(first)
            layout:add(second)
            layout:add(third)
        end)

        describe("with enough space", function()
            it("fit", function()
                assert.widget_fit(layout, { 100, 100 }, { 15, 35 })
            end)

            it("layout", function()
                assert.widget_layout(layout, { 100, 100 }, {
                    p(first,  0,  0, 100, 33),
                    p(second, 0, 33, 100, 33),
                    p(third,  0, 67, 100, 33),
                })
            end)
        end)

        describe("without enough height", function()
            it("fit", function()
                assert.widget_fit(layout, { 5, 100 }, { 5, 35 })
            end)

            it("layout", function()
                assert.widget_layout(layout, { 5, 100 }, {
                    p(first,  0,  0, 5, 33),
                    p(second, 0, 33, 5, 33),
                    p(third,  0, 67, 5, 33),
                })
            end)
        end)

        describe("without enough width", function()
            it("fit", function()
                assert.widget_fit(layout, { 100, 20 }, { 15, 20 })
            end)

            it("layout", function()
                assert.widget_layout(layout, { 100, 20 }, {
                    p(first,  0,  0, 100, 6),
                    p(second, 0,  7, 100, 6),
                    p(third,  0, 13, 100, 6),
                })
            end)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
