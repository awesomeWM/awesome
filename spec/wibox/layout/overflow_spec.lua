---------------------------------------------------------------------------
-- @author Lucas Schwiderski
-- @copyright 2021 Lucas Schwiderski
---------------------------------------------------------------------------

local spy = require("luassert.spy")
local match = require("luassert.match")
local overflow = require("wibox.layout.overflow")
local base = require("wibox.widget.base")
local utils = require("wibox.test_utils")
local p = require("wibox.widget.base").place_widget_at

describe("wibox.layout.overflow.vertical", function()
    local layout
    before_each(function()
        layout = overflow.vertical()
    end)

    describe(":add", function()
        it("fails without parameters", function()
            assert.has_error(function()
                layout:add()
            end)
        end)

        it("adds children", function()
            local w1, w2 = base.empty_widget(), base.empty_widget()

            assert.is.same({}, layout:get_children())

            layout:add(w1)
            assert.is.same({ w1 }, layout:get_children())

            layout:add(w2, w2)
            assert.is.same({ w1, w2, w2 }, layout:get_children())
        end)

        it("emits signal 'widget::layout_changed'", function()
            local w1 = base.empty_widget()
            local s = spy(function() end)
            layout:connect_signal("widget::layout_changed", function(...)
                s(...)
            end)

            layout:add(w1)
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)
    end)

    describe(":reset", function()
        it("removes all widgets", function()
            local w1, w2 = base.empty_widget(), base.empty_widget()

            layout:add(w1, w2)
            assert.is.same({ w1, w2 }, layout:get_children())

            layout:reset()
            assert.is.same({}, layout:get_children())
        end)

        it("does not fail when already empty", function()
            layout:reset()
        end)

        it("emits signal 'widget::layout_changed'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::layout_changed", function(...)
                s(...)
            end)

            layout:reset()
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)

        it("emits signal 'widget::reseted'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::reseted", function(...)
                s(...)
            end)

            layout:reset()
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)

        it("emits signal 'widget::reset'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::reset", function(...)
                s(...)
            end)

            layout:reset()
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)
    end)

    describe(":fill_space", function()
        it("emits signal 'widget::layout_changed'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::layout_changed", function(...)
                s(...)
            end)

            layout:fill_space(false)
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)

        it("emits signal 'property::fill_space'", function()
            local s = spy(function() end)
            layout:connect_signal("property::fill_space", function(...)
                s(...)
            end)

            -- Since the default is `true`, we must use `false` for the desired
            -- effect
            layout:fill_space(false)
            assert.spy(s).was.called_with(match.is_ref(layout), match.is_false())

            -- Assigning the same value again should not do anything
            layout:fill_space(false)
            assert.spy(s).was.called(1)
        end)
    end)

    describe("spacing", function()
        it("sets the property", function()
            layout.spacing = 10
            assert.is_equal(10, layout.spacing)
        end)

        it("emits signal 'widget::layout_changed'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::layout_changed", function(...)
                s(...)
            end)

            layout.spacing = 10
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)

        it("emits signal 'property::spacing'", function()
            local s = spy(function() end)
            layout:connect_signal("property::spacing", function(...)
                s(...)
            end)

            layout.spacing = 10
            assert.spy(s).was.called_with(match.is_ref(layout), 10)

            -- Assigning the same value again should not do anything
            layout.spacing = 10
            assert.spy(s).was.called(1)
        end)
    end)

    describe(":fit", function()
        it("handles empty content", function()
            assert.widget_fit(layout, { 10, 30 }, { 0, 0 })
        end)

        describe("with content", function()
            local first, second, third

            before_each(function()
                first = utils.widget_stub(10, 10)
                second = utils.widget_stub(15, 15)
                third = utils.widget_stub(10, 10)

                layout:add(first, second, third)
            end)

            it("handles enough space", function()
                assert.widget_fit(layout, { 100, 100 }, { 15, 35 })
            end)

            it("handles missing width", function()
                assert.widget_fit(layout, { 5, 100 }, { 5, 35 })
            end)

            it("handles missing height", function()
                assert.widget_fit(layout, { 30, 20 }, { 20, 20 })
            end)
        end)
    end)

    describe(":layout", function()
        it("handles empty content", function()
            assert.widget_layout(layout, { 30, 30 }, {})
        end)

        describe("with content", function()
            local first, second, third

            before_each(function()
                first = utils.widget_stub(10, 10)
                second = utils.widget_stub(15, 15)
                third = utils.widget_stub(10, 10)

                layout:add(first, second, third)
            end)

            it("handles enough space", function()
                assert.widget_layout(layout, { 30, 100 }, {
                    p(first,  0,  0, 30, 10),
                    p(second, 0, 10, 30, 15),
                    p(third,  0, 25, 30, 10),
                })
            end)

            it("handles missing width", function()
                assert.widget_layout(layout, { 5, 100 }, {
                    p(first,  0,  0, 5, 10),
                    p(second, 0, 10, 5, 15),
                    p(third,  0, 25, 5, 10),
                })
            end)

            it("handles missing height", function()
                local scrollbar = utils.widget_stub(10, 10)
                layout:set_scrollbar_widget(scrollbar)

                assert.widget_layout(layout, { 30, 20 }, {
                    p(scrollbar, 25,  0,  5, 10),
                    p(first,      0,  0, 25, 10),
                    p(second,     0, 10, 25, 15),
                })
            end)
        end)
    end)

    describe("scrolling", function()
        local first, second, third
        local scrollbar = utils.widget_stub(10, 10)

        before_each(function()
            first = utils.widget_stub(10, 10)
            second = utils.widget_stub(15, 15)
            third = utils.widget_stub(10, 10)

            layout:add(first, second, third)
            layout:set_scrollbar_widget(scrollbar)
        end)

        it("scrolls to end", function()
            assert.widget_layout(layout, { 100, 20 }, {
                p(scrollbar,   95,  0,  5, 10),
                p(first,  0,  0, 95, 10),
                p(second, 0, 10, 95, 15),
            })

            layout:set_scroll_factor(1)

            assert.widget_layout(layout, { 100, 20 }, {
                p(scrollbar,   95,  9,  5, 10),
                p(first,  0,  -15, 95, 10),
                p(second, 0, -5, 95, 15),
                p(third, 0, 10, 95, 10),
            })
        end)

        it("scrolls one step", function()
            assert.widget_layout(layout, { 100, 20 }, {
                p(scrollbar,   95,  0,  5, 10),
                p(first,  0,  0, 95, 10),
                p(second, 0, 10, 95, 15),
            })

            layout:scroll(1)

            assert.widget_layout(layout, { 100, 20 }, {
                p(scrollbar,   95,  2,  5, 10),
                p(first,  0,  -5, 95, 10),
                p(second, 0, 5, 95, 15),
            })
        end)
    end)
end)

describe("wibox.layout.overflow.horizontal", function()
    local layout
    before_each(function()
        layout = overflow.horizontal()
    end)

    describe(":add", function()
        it("fails without parameters", function()
            assert.has_error(function()
                layout:add()
            end)
        end)

        it("adds children", function()
            local w1, w2 = base.empty_widget(), base.empty_widget()

            assert.is.same({}, layout:get_children())

            layout:add(w1)
            assert.is.same({ w1 }, layout:get_children())

            layout:add(w2, w2)
            assert.is.same({ w1, w2, w2 }, layout:get_children())
        end)

        it("emits signal 'widget::layout_changed'", function()
            local w1 = base.empty_widget()
            local s = spy(function() end)
            layout:connect_signal("widget::layout_changed", function(...)
                s(...)
            end)

            layout:add(w1)
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)
    end)

    describe(":reset", function()
        it("removes all widgets", function()
            local w1, w2 = base.empty_widget(), base.empty_widget()

            layout:add(w1, w2)
            assert.is.same({ w1, w2 }, layout:get_children())

            layout:reset()
            assert.is.same({}, layout:get_children())
        end)

        it("does not fail when already empty", function()
            layout:reset()
        end)

        it("emits signal 'widget::layout_changed'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::layout_changed", function(...)
                s(...)
            end)

            layout:reset()
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)

        it("emits signal 'widget::reseted'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::reseted", function(...)
                s(...)
            end)

            layout:reset()
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)

        it("emits signal 'widget::reset'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::reset", function(...)
                s(...)
            end)

            layout:reset()
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)
    end)

    describe(":fill_space", function()
        it("emits signal 'widget::layout_changed'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::layout_changed", function(...)
                s(...)
            end)

            layout:fill_space(false)
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)

        it("emits signal 'property::fill_space'", function()
            local s = spy(function() end)
            layout:connect_signal("property::fill_space", function(...)
                s(...)
            end)

            -- Since the default is `true`, we must use `false` for the desired
            -- effect
            layout:fill_space(false)
            assert.spy(s).was.called_with(match.is_ref(layout), match.is_false())

            -- Assigning the same value again should not do anything
            layout:fill_space(false)
            assert.spy(s).was.called(1)
        end)
    end)

    describe("spacing", function()
        it("sets the property", function()
            layout.spacing = 10
            assert.is_equal(10, layout.spacing)
        end)

        it("emits signal 'widget::layout_changed'", function()
            local s = spy(function() end)
            layout:connect_signal("widget::layout_changed", function(...)
                s(...)
            end)

            layout.spacing = 10
            assert.spy(s).was.called_with(match.is_ref(layout))
        end)

        it("emits signal 'property::spacing'", function()
            local s = spy(function() end)
            layout:connect_signal("property::spacing", function(...)
                s(...)
            end)

            layout.spacing = 10
            assert.spy(s).was.called_with(match.is_ref(layout), 10)

            -- Assigning the same value again should not do anything
            layout.spacing = 10
            assert.spy(s).was.called(1)
        end)
    end)

    describe(":fit", function()
        it("handles empty content", function()
            assert.widget_fit(layout, { 10, 30 }, { 0, 0 })
        end)

        describe("with content", function()
            local first, second, third

            before_each(function()
                first = utils.widget_stub(10, 10)
                second = utils.widget_stub(15, 15)
                third = utils.widget_stub(10, 10)

                layout:add(first, second, third)
            end)

            it("handles enough space", function()
                assert.widget_fit(layout, { 100, 100 }, { 35, 15 })
            end)

            it("handles missing width", function()
                assert.widget_fit(layout, { 20, 30 }, { 20, 20 })
            end)

            it("handles missing height", function()
                assert.widget_fit(layout, { 100, 5 }, { 35, 5 })
            end)
        end)
    end)

    describe(":layout", function()
        it("handles empty content", function()
            assert.widget_layout(layout, { 30, 30 }, {})
        end)

        describe("with content", function()
            local first, second, third

            before_each(function()
                first = utils.widget_stub(10, 10)
                second = utils.widget_stub(15, 15)
                third = utils.widget_stub(10, 10)

                layout:add(first, second, third)
            end)

            it("handles enough space", function()
                assert.widget_layout(layout, { 100, 30 }, {
                    p(first,   0, 0, 10, 30),
                    p(second, 10, 0, 15, 30),
                    p(third,  25, 0, 10, 30),
                })
            end)

            it("handles missing width", function()
                local scrollbar = utils.widget_stub(11, 11)
                layout:set_scrollbar_widget(scrollbar)

                assert.widget_layout(layout, { 20, 30 }, {
                    p(scrollbar,  0, 25, 11,  5),
                    p(first,      0,  0, 10, 25),
                    p(second,    10,  0, 15, 25),
                })
            end)

            it("handles missing height", function()
                assert.widget_layout(layout, { 100, 20 }, {
                    p(first,   0, 0, 10, 20),
                    p(second, 10, 0, 15, 20),
                    p(third,  25, 0, 10, 20),
                })
            end)
        end)
    end)

    describe("scrolling", function()
        local first, second, third
        local scrollbar = utils.widget_stub(10, 10)

        before_each(function()
            first = utils.widget_stub(10, 10)
            second = utils.widget_stub(15, 15)
            third = utils.widget_stub(10, 10)

            layout:add(first, second, third)
            layout:set_scrollbar_widget(scrollbar)
        end)

        it("scrolls to end", function()
            assert.widget_layout(layout, { 20, 30 }, {
                p(scrollbar, 0, 25, 10,  5),
                p(first,     0,  0, 10, 25),
                p(second,   10,  0, 15, 25),
            })

            layout:set_scroll_factor(1)

            assert.widget_layout(layout, { 20, 30 }, {
                p(scrollbar, 9, 25, 10,  5),
                p(second,   -5,  0, 15, 25),
                p(third,    10,  0, 10, 25),
            })
        end)

        it("scrolls one step", function()
            assert.widget_layout(layout, { 20, 30 }, {
                p(scrollbar,  0, 25, 10,  5),
                p(first,      0,  0, 10, 25),
                p(second,    10,  0, 15, 25),
            })

            layout:scroll(1)

            assert.widget_layout(layout, { 20, 30 }, {
                p(scrollbar,  2, 25, 10,  5),
                p(first,     -5,  0, 10, 25),
                p(second,     5,  0, 15, 25),
            })
        end)
    end)
end)

describe("wibox.layout.overflow", function()
    local layout
    before_each(function()
        layout = overflow.vertical()
    end)

    it("handles `wibox.layout.fixed` as child", function()
        local fixed = require("wibox.layout.fixed")
        local w1 = utils.widget_stub(10, 10)
        local w2 = utils.widget_stub(10, 10)
        local lfixed = fixed.vertical()
        lfixed:add(w1)
        lfixed:add(w2)

        layout:add(lfixed)

        assert.widget_layout(layout, { 100, 100 }, {
            p(lfixed, 0, 0, 100, 20),
        })
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
