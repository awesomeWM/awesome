---------------------------------------------------------------------------
-- @author Aire-One
-- @copyright 2021 Aire-One
---------------------------------------------------------------------------

_G.awesome.connect_signal = function() end

local gtimer = require("gears.timer")
local template = require("wibox.template")
local base = require("wibox.widget.base").make_widget

describe("wibox.template", function()
    local widget

    before_each(function()
        widget = template()
    end)

    describe(".new()", function()
        it("update_now", function()
            local spied_update_callback = spy.new(function() end)

            template {
                update_callback = function(...) spied_update_callback(...) end,
                update_now = true,
            }

            gtimer.run_delayed_calls_now()
            assert.spy(spied_update_callback).was.called()
        end)
    end)

    describe(":update()", function()
        it("batch calls", function()
            local spied_update_callback = spy.new(function() end)

            widget.update_callback = function(...) spied_update_callback(...) end

            -- Multiple calls to update
            widget:update()
            widget:update()
            widget:update()

            -- update_callback shouldn't be called before the end of the event loop
            assert.spy(spied_update_callback).was.called(0)

            gtimer.run_delayed_calls_now()

            -- updates are batched, so only 1 call should have been performed
            assert.spy(spied_update_callback).was.called(1)
        end)

        it("update parameters", function()
            local spied_update_callback = spy.new(function() end)
            local args = { foo = "string" }

            widget.update_callback = function(...) spied_update_callback(...) end

            widget:update(args)

            gtimer.run_delayed_calls_now()

            assert.spy(spied_update_callback).was.called_with(
                match.is_ref(widget),
                match.is_same(args)
            )
        end)

        it("crush update parameters", function()
            local spied_update_callback = spy.new(function() end)

            widget.update_callback = function(...) spied_update_callback(...) end

            widget:update { foo = "bar" }
            widget:update { bar = 10 }

            gtimer.run_delayed_calls_now()

            assert.spy(spied_update_callback).was.called_with(
                match.is_ref(widget),
                match.is_same { foo = "bar", bar = 10 }
            )
        end)

        it("set_property", function()
            local called

            widget = template {
                {
                    id     = "one",
                    foo    = "bar",
                    widget = base
                },
                {
                    id         = "two",
                    everything = 42,
                    widget     = base
                },
                {
                    id         = "three",
                    set_prop   = function(_, val)
                        called = val
                    end,
                    widget     = base
                },
                id     = "main",
                widget = base,
            }

            assert.is.equal(widget:get_children_by_id("main")[1], widget.children[1])
            assert.is.equal(widget:get_children_by_id("one")[1].foo, "bar")
            assert.is.equal(widget:get_children_by_id("two")[1].everything, 42)
            assert.is.equal(called, nil)

            widget:set_property("prop", 1337, "three")
            assert.is.equal(called, 1337)

            widget:set_property("foo", "baz", "one")
            assert.is.equal(widget:get_children_by_id("one")[1].foo, "baz")

            widget:set_property("everything", -42, "two")
            assert.is.equal(widget:get_children_by_id("two")[1].everything, -42)

            widget:set_property("foobar", true, {"one", "two"})
            assert.is_true(widget:get_children_by_id("one")[1].foobar)
            assert.is_true(widget:get_children_by_id("two")[1].foobar)
            assert.is_nil(widget:get_children_by_id("three")[1].foobar)

            widget:set_property("test", 1337)
            assert.is.equal(widget:get_children_by_id("main")[1].test, 1337)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
