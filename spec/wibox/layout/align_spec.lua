---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local align = require("wibox.layout.align")
local utils = require("wibox.test_utils")

describe("wibox.layout.flex", function()
    before_each(utils.stub_draw_widget)
    after_each(utils.revert_draw_widget)

    describe("expand=none", function()
        local layout
        before_each(function()
            layout = align.vertical()
            layout:set_expand("none")
        end)

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

                layout:set_first(first)
                layout:set_second(second)
                layout:set_third(third)
            end)

            describe("with enough space", function()
                it("fit", function()
                    assert.widget_fit(layout, { 100, 100 }, { 15, 35 })
                end)

                it("draw", function()
                    layout:draw("wibox", "cr", 100, 100)
                    utils.check_widgets_drawn({
                        { first,  0,  0, 100, 10 },
                        { third,  0, 90, 100, 10 },
                        { second, 0, 42, 100, 15 },
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
                        { third,  0, 90, 5, 10 },
                        { second, 0, 42, 5, 15 },
                    })
                end)
            end)

            describe("without enough width", function()
                it("fit", function()
                    -- XXX: Is this really what should happen?
                    assert.widget_fit(layout, { 100, 20 }, { 15, 35 })
                end)

                it("draw", function()
                    layout:draw("wibox", "cr", 100, 20)
                    --- XXX: Shouldn't this also draw part of the second widget?
                    utils.check_widgets_drawn({
                        { first,  0,  0, 100, 10 },
                        { third,  0, 10, 100, 10 },
                        { second, 0,  2, 100, 15 },
                    })
                end)
            end)
        end)
    end)

    describe("expand=outside", function()
        local layout
        before_each(function()
            layout = align.vertical()
            layout:set_expand("outside")
        end)

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

                layout:set_first(first)
                layout:set_second(second)
                layout:set_third(third)
            end)

            describe("with enough space", function()
                it("fit", function()
                    assert.widget_fit(layout, { 100, 100 }, { 15, 35 })
                end)

                it("draw", function()
                    layout:draw("wibox", "cr", 100, 100)
                    utils.check_widgets_drawn({
                        { first,  0,  0, 100, 42 },
                        { third,  0, 58, 100, 42 },
                        { second, 0, 42, 100, 15 },
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
                        { first,  0,  0, 5, 42 },
                        { third,  0, 58, 5, 42 },
                        { second, 0, 42, 5, 15 },
                    })
                end)
            end)

            describe("without enough width", function()
                it("fit", function()
                    -- XXX: Is this really what should happen?
                    assert.widget_fit(layout, { 100, 20 }, { 15, 35 })
                end)

                it("draw", function()
                    layout:draw("wibox", "cr", 100, 20)
                    utils.check_widgets_drawn({
                        { first,  0,  0, 100, 2 },
                        { third,  0, 18, 100, 2 },
                        { second, 0,  2, 100, 15 },
                    })
                end)
            end)
        end)
    end)

    describe("expand=inside", function()
        local layout
        before_each(function()
            layout = align.vertical()
            layout:set_expand("inside")
        end)

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

                layout:set_first(first)
                layout:set_second(second)
                layout:set_third(third)
            end)

            describe("with enough space", function()
                it("fit", function()
                    assert.widget_fit(layout, { 100, 100 }, { 15, 35 })
                end)

                it("draw", function()
                    layout:draw("wibox", "cr", 100, 100)
                    utils.check_widgets_drawn({
                        { first,  0,  0, 100, 10 },
                        { third,  0, 90, 100, 10 },
                        { second, 0, 10, 100, 80 },
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
                        { third,  0, 90, 5, 10 },
                        { second, 0, 10, 5, 80 },
                    })
                end)
            end)

            describe("without enough width", function()
                it("fit", function()
                    -- XXX: Is this really what should happen?
                    assert.widget_fit(layout, { 100, 20 }, { 15, 35 })
                end)

                it("draw", function()
                    layout:draw("wibox", "cr", 100, 20)
                    --- XXX: Shouldn't this also draw part of the second widget?
                    utils.check_widgets_drawn({
                        { first,  0,  0, 100, 10 },
                        { third,  0, 10, 100, 10 },
                    })
                end)
            end)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
