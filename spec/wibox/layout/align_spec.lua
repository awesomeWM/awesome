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
                        p(first,  0,  0, 100,  2),
                        p(third,  0, 18, 100,  2),
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

    describe("emitting signals", function()
        local layout, layout_changed
        before_each(function()
            layout = align.vertical()
            layout:connect_signal("widget::layout_changed", function()
                layout_changed = layout_changed + 1
            end)
            layout_changed = 0
        end)

        it("set first", function()
            local w1, w2 = {}, {}
            assert.is.equal(layout_changed, 0)
            layout:set_first(w1)
            assert.is.equal(layout_changed, 1)
            layout:set_first(w2)
            assert.is.equal(layout_changed, 2)
            layout:set_first(w2)
            assert.is.equal(layout_changed, 2)
        end)

        it("set second", function()
            local w1, w2 = {}, {}
            assert.is.equal(layout_changed, 0)
            layout:set_second(w1)
            assert.is.equal(layout_changed, 1)
            layout:set_second(w2)
            assert.is.equal(layout_changed, 2)
            layout:set_second(w2)
            assert.is.equal(layout_changed, 2)
        end)

        it("set third", function()
            local w1, w2 = {}, {}
            assert.is.equal(layout_changed, 0)
            layout:set_third(w1)
            assert.is.equal(layout_changed, 1)
            layout:set_third(w2)
            assert.is.equal(layout_changed, 2)
            layout:set_third(w2)
            assert.is.equal(layout_changed, 2)
        end)

        it("set again", function()
            local w1, w2, w3 = {}, {}, {}
            layout = align.vertical(w1, w2, w3)
            layout:connect_signal("widget::layout_changed", function()
                layout_changed = layout_changed + 1
            end)
            assert.is.equal(layout_changed, 0)
            layout:set_first(w1)
            layout:set_second(w2)
            layout:set_third(w3)
            assert.is.equal(layout_changed, 0)
        end)
    end)

    it("set_children", function()
        local w1, w2, w3 = { w1 = true }, { w2 = true }, { w3 = true }
        local layout = align.vertical()

        assert.is.same({}, layout:get_children())

        layout:set_second(w2)
        assert.is.same({ w2 }, layout:get_children())

        layout:set_first(w1)
        assert.is.same({ w1, w2 }, layout:get_children())

        layout:set_third(w3)
        assert.is.same({ w1, w2, w3 }, layout:get_children())

        layout:set_second(nil)
        assert.is.same({ w1, w3 }, layout:get_children())

        layout:reset()
        assert.is.same({}, layout:get_children())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
