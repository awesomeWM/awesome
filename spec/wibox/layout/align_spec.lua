---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
---------------------------------------------------------------------------

local align = require("wibox.layout.align")
local utils = require("wibox.test_utils")
local p = require("wibox.widget.base").place_widget_at

describe("wibox.layout.align", function()
    describe("expand=none", function()
        local layout
        before_each(function()
            layout = align.vertical()
            layout:set_expand("none")
        end)

        it("empty layout fit", function()
            assert.widget_fit(layout, { 10, 10 }, { 0, 0 })
        end)

        it("empty layout layout", function()
            assert.is.same({}, layout:layout(0, 0))
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

                it("layout", function()
                    assert.widget_layout(layout, { 100, 100 }, {
                        p(first,  0, 0, 100, 10),
                        p(third,  0, 90, 100, 10),
                        p(second, 0, 42, 100, 15),
                    })
                end)
            end)

            describe("without enough height", function()
                it("fit", function()
                    assert.widget_fit(layout, { 5, 100 }, { 5, 35 })
                end)

                it("layout", function()
                    assert.widget_layout(layout, { 5, 100 }, {
                        p(first,  0,  0, 5, 10),
                        p(third,  0, 90, 5, 10),
                        p(second, 0, 42, 5, 15),
                    })
                end)
            end)

            describe("without enough width", function()
                it("fit", function()
                    assert.widget_fit(layout, { 100, 20 }, { 15, 20 })
                end)

                it("layout", function()
                    assert.widget_layout(layout, { 100, 20 }, {
                        p(first,  0,  0, 100, 10),
                        p(third,  0, 10, 100, 10),
                        p(second, 0,  2, 100, 15),
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

        it("empty layout layout", function()
            assert.widget_layout(layout, { 0, 0 }, {})
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

                it("layout", function()
                    assert.widget_layout(layout, { 100, 100 }, {
                        p(first,  0,  0, 100, 42),
                        p(third,  0, 58, 100, 42),
                        p(second, 0, 42, 100, 15),
                    })
                end)
            end)

            describe("without enough height", function()
                it("fit", function()
                    assert.widget_fit(layout, { 5, 100 }, { 5, 35 })
                end)

                it("layout", function()
                    assert.widget_layout(layout, { 5, 100 }, {
                        p(first,  0,  0, 5, 42),
                        p(third,  0, 58, 5, 42),
                        p(second, 0, 42, 5, 15),
                    })
                end)
            end)

            describe("without enough width", function()
                it("fit", function()
                    assert.widget_fit(layout, { 100, 20 }, { 15, 20 })
                end)

                it("layout", function()
                    assert.widget_layout(layout, { 100, 20 }, {
                        p(first,  0,  0, 100, 2),
                        p(third,  0, 18, 100, 2),
                        p(second, 0,  2, 100, 15),
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

        it("empty layout layout", function()
            assert.widget_layout(layout, { 0, 0 }, {})
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

                it("layout", function()
                    assert.widget_layout(layout, { 100, 100 }, {
                        p(first,  0,  0, 100, 10),
                        p(third,  0, 90, 100, 10),
                        p(second, 0, 10, 100, 80),
                    })
                end)
            end)

            describe("without enough height", function()
                it("fit", function()
                    assert.widget_fit(layout, { 5, 100 }, { 5, 35 })
                end)

                it("layout", function()
                    assert.widget_layout(layout, { 5, 100 }, {
                        p(first,  0,  0, 5, 10),
                        p(third,  0, 90, 5, 10),
                        p(second, 0, 10, 5, 80),
                    })
                end)
            end)

            describe("without enough width", function()
                it("fit", function()
                    assert.widget_fit(layout, { 100, 20 }, { 15, 20 })
                end)

                it("layout", function()
                    assert.widget_layout(layout, { 100, 20 }, {
                        p(first,  0,  0, 100, 10),
                        p(third,  0, 10, 100, 10),
                    })
                end)
            end)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
