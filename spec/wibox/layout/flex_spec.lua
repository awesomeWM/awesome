---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local flex = require("wibox.layout.flex")
local utils = require("wibox.test_utils")

describe("wibox.layout.flex", function()
    local layout
    before_each(function()
        layout = flex.vertical()
    end)

    before_each(utils.stub_draw_widget)
    after_each(utils.revert_draw_widget)

    it("empty layout fit", function()
        assert.widget_fit(layout, { 10, 10 }, { 0, 0 })
        utils.check_widgets_drawn({})
    end)

    it("empty layout draw", function()
        layout:draw(nil, nil, 0, 0)
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

            it("draw", function()
                layout:draw("wibox", "cr", 100, 100)
                utils.check_widgets_drawn({
                    { first,  0,  0, 100, 33 },
                    { second, 0, 33, 100, 33 },
                    { third,  0, 67, 100, 33 },
                })
            end)
        end)

        describe("without enough height", function()
            it("fit", function()
                assert.widget_fit(layout, { 5, 100 }, { 5, 35 })
            end)

            it("draw", function()
                layout:draw("wibox", "cr", 5, 100)
                utils.check_widgets_drawn({
                    { first,  0,  0, 5, 33 },
                    { second, 0, 33, 5, 33 },
                    { third,  0, 67, 5, 33 },
                })
            end)
        end)

        describe("without enough width", function()
            it("fit", function()
                assert.widget_fit(layout, { 100, 20 }, { 15, 20 })
            end)

            it("draw", function()
                layout:draw("wibox", "cr", 100, 20)
                utils.check_widgets_drawn({
                    { first,  0,  0, 100, 6 },
                    { second, 0,  7, 100, 6 },
                    { third,  0, 13, 100, 6 },
                })
            end)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
