---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local fixed = require("wibox.layout.fixed")
local utils = require("wibox.test_utils")

describe("wibox.layout.fixed", function()
    local layout
    before_each(function()
        layout = fixed.vertical()
    end)

    before_each(utils.stub_draw_widget)
    after_each(utils.revert_draw_widget)

    it("empty layout fit", function()
        assert.widget_fit(layout, { 10, 10 }, { 0, 0 })
    end)

    it("empty layout draw", function()
        layout:draw(nil, nil, 0, 0)
        utils.check_widgets_drawn({})
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
                    { first,  0,  0, 100, 10 },
                    { second, 0, 10, 100, 15 },
                    { third,  0, 25, 100, 10 },
                })
            end)
        end)

        describe("without enough height", function()
            it("fit", function()
                -- XXX: Is this really what should happen?
                assert.widget_fit(layout, { 5, 100 }, { 15, 35 })
            end)

            it("draw", function()
                layout:draw("wibox", "cr", 5, 100)
                utils.check_widgets_drawn({
                    { first,  0,  0, 5, 10 },
                    { second, 0, 10, 5, 15 },
                    { third,  0, 25, 5, 10 },
                })
            end)
        end)

        describe("without enough width", function()
            it("fit", function()
                -- XXX: Is this really what should happen?
                assert.widget_fit(layout, { 100, 20 }, { 15, 20 })
            end)

            it("draw", function()
                layout:draw("wibox", "cr", 100, 20)
                --- XXX: Shouldn't this also draw part of the second widget?
                utils.check_widgets_drawn({
                    { first,  0,  0, 100, 10 },
                })
            end)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
