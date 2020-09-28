local stack = require("wibox.layout.stack")
local base = require("wibox.widget.base")
local utils = require("wibox.test_utils")
local p = require("wibox.widget.base").place_widget_at

describe("wibox.layout.stack", function()
    local layout
    before_each(function()
        layout = stack()
    end)

    it("empty layout fit", function()
        assert.widget_fit(layout, { 10, 10 }, { 0, 0 })
    end)

    it("empty layout layout", function()
        assert.widget_layout(layout, { 0, 0 }, {})
    end)

    it("empty add", function()
        assert.has_error(function()
            layout:add()
        end)
    end)

    describe("with widgets", function()
        local first, second, third

        before_each(function()
            first = utils.widget_stub(10, 10)
            second = utils.widget_stub(15, 15)
            third = utils.widget_stub(10, 10)

            layout:add(first, second, third)
        end)

        describe("with enough space", function()
            it("fit", function()
                assert.widget_fit(layout, { 100, 100 }, { 15, 35 })
            end)

            it("layout", function()
                assert.widget_layout(layout, { 100, 100 }, {
                    p(first,  0,  0, 100, 10),
                    p(second, 0, 10, 100, 15),
                    p(third,  0, 25, 100, 10),
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
                    p(second, 0, 10, 5, 15),
                    p(third,  0, 25, 5, 10),
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
                    p(second, 0, 10, 100, 10),
                    p(third,  0, 20, 100,  0),
                })
            end)
        end)
    end)

    describe("emitting signals", function()
        local layout_changed
        before_each(function()
            layout:connect_signal("widget::layout_changed", function()
                layout_changed = layout_changed + 1
            end)
            layout_changed = 0
        end)

        it("add", function()
            local w1, w2 = base.empty_widget(), base.empty_widget()
            assert.is.equal(layout_changed, 0)
            layout:add(w1)
            assert.is.equal(layout_changed, 1)
            layout:add(w2)
            assert.is.equal(layout_changed, 2)
            layout:add(w2)
            assert.is.equal(layout_changed, 3)
        end)

        it("set_spacing", function()
            assert.is.equal(layout_changed, 0)
            layout:set_spacing(0)
            assert.is.equal(layout_changed, 0)
            layout:set_spacing(5)
            assert.is.equal(layout_changed, 1)
            layout:set_spacing(2)
            assert.is.equal(layout_changed, 2)
            layout:set_spacing(2)
            assert.is.equal(layout_changed, 2)
        end)

        it("reset", function()
            assert.is.equal(layout_changed, 0)
            layout:add(base.make_widget())
            assert.is.equal(layout_changed, 1)
            layout:reset()
            assert.is.equal(layout_changed, 2)
        end)

        it("fill_space", function()
            assert.is.equal(layout_changed, 0)
            layout:fill_space(false)
            assert.is.equal(layout_changed, 0)
            layout:fill_space(true)
            assert.is.equal(layout_changed, 1)
            layout:fill_space(true)
            assert.is.equal(layout_changed, 1)
            layout:fill_space(false)
            assert.is.equal(layout_changed, 2)
        end)
    end)

    it("set_children", function()
        local w1, w2 = base.empty_widget(), base.empty_widget()

        assert.is.same({}, layout:get_children())

        layout:add(w1)
        assert.is.same({ w1 }, layout:get_children())

        layout:add(w2)
        assert.is.same({ w1, w2 }, layout:get_children())

        layout:reset()
        assert.is.same({}, layout:get_children())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
