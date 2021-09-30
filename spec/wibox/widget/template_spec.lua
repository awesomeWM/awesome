---------------------------------------------------------------------------
-- @author Aire-One
-- @copyright 2021 Aire-One
---------------------------------------------------------------------------

_G.awesome.connect_signal = function() end

local template = require("wibox.widget.template")
local gtimer = require("gears.timer")

describe("wibox.widget.template", function()
    local widget

    before_each(function()
        widget = template()
    end)

    describe("widget:update()", function()
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

    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
