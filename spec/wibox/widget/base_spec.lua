---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2016 Uli Schlachter
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local no_parent = base.no_parent_I_know_what_I_am_doing

describe("wibox.widget.base", function()
    local widget1, widget2
    before_each(function()
        widget1 = base.make_widget()
        widget2 = base.make_widget()
        widget1.layout = function()
            return { base.place_widget_at(widget2, 0, 0, 1, 1) }
        end
    end)

    describe("caches", function()
        it("garbage collectable", function()
            local alive = setmetatable({ widget1, widget2 }, { __mode = "kv" })
            assert.is.equal(2, #alive)

            widget1, widget2 = nil, nil
            collectgarbage("collect")
            assert.is.equal(0, #alive)
        end)

        it("simple cache clear", function()
            local alive = setmetatable({ widget1, widget2 }, { __mode = "kv" })
            base.layout_widget(no_parent, { "fake context" }, widget1, 20, 20)
            assert.is.equal(2, #alive)

            widget1, widget2 = nil, nil
            collectgarbage("collect")
            assert.is.equal(0, #alive)
        end)

        it("self-reference cache clear", function()
            widget2.widget = widget1

            local alive = setmetatable({ widget1, widget2 }, { __mode = "kv" })
            base.layout_widget(no_parent, { "fake context" }, widget1, 20, 20)
            assert.is.equal(2, #alive)

            widget1, widget2 = nil, nil
            collectgarbage("collect")
            assert.is.equal(0, #alive)
        end)
    end)

    describe("setup", function()
        it("Filters out 'false'", function()
            -- Regression test: There was a bug where "nil"s where correctly
            -- skipped, but "false" entries survived
            local layout1, layout2 = base.make_widget(), base.make_widget()
            local called = false
            function layout1:set_widget(w)
                called = true
                assert.equals(w, layout2)
            end
            function layout2:set_children(children)
                assert.is_same({nil, widget1, nil, widget2}, children)
            end
            layout2.allow_empty_widget = true
            layout1:setup{ layout = layout2, false, widget1, nil, widget2 }
            assert.is_true(called)
        end)

        it("Attribute 'false' works", function()
            -- Regression test: I introduced a bug with the above fix
            local layout1, layout2 = base.make_widget(), base.make_widget()
            local called1, called2 = false, false
            function layout1:set_widget(w)
                called1 = true
                assert.equals(w, layout2)
            end
            function layout2:set_children(children)
                assert.is_same({}, children)
            end
            function layout2:set_foo(foo)
                called2 = true
                assert.is_false(foo)
            end
            layout1:setup{ layout = layout2, foo = false }
            assert.is_true(called1)
            assert.is_true(called2)
        end)
    end)

    describe("Setters emit signals", function()
        local widget
        local signal_called
        local callback = function () signal_called = true end

        before_each(function()
            widget = base.make_widget()
            signal_called = false
        end)

        it("'set_visible' calls 'widget::layout_changed'", function()
            widget:connect_signal("widget::layout_changed", callback)
            widget:set_visible(false)
            assert.is_true(signal_called)
        end)

        it("'set_visible' calls 'widget::redraw_needed'", function()
            widget:connect_signal("widget::redraw_needed", callback)
            widget:set_visible(false)
            assert.is_true(signal_called)
        end)

        it("'set_opacity' calls 'widget::redraw_needed'", function()
            widget:connect_signal("widget::redraw_needed", callback)
            widget:set_opacity(0)
            assert.is_true(signal_called)
        end)

        it("'set_forced_width' calls 'widget::layout_changed'", function()
            widget:connect_signal("widget::layout_changed", callback)
            widget:set_forced_width(0)
            assert.is_true(signal_called)
        end)

        it("'set_forced_height' calls 'widget::layout_changed'", function()
            widget:connect_signal("widget::layout_changed", callback)
            widget:set_forced_height(0)
            assert.is_true(signal_called)
        end)

        it("'set_widget_common' calls 'property::widget'", function()
            -- Implement `set_widget`
            rawset(widget, "set_widget", base.set_widget_common)

            widget:connect_signal("property::widget", callback)
            widget:set_widget(base.make_widget())
            assert.is_true(signal_called)
        end)

        it("'set_widget_common' calls 'widget::layout_changed'", function()
            -- Implement `set_widget`
            rawset(widget, "set_widget", base.set_widget_common)

            widget:connect_signal("widget::layout_changed", callback)
            widget:set_widget(base.make_widget())
            assert.is_true(signal_called)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
