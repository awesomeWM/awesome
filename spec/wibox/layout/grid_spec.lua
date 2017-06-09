---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local grid = require("wibox.layout.grid")
local base = require("wibox.widget.base")

describe("wibox.layout.grid", function()
    it("set_children", function()
        local layout = grid()
        local w1, w2 = base.empty_widget(), base.empty_widget()

        assert.is.same({}, layout:get_children())

        layout:add(w1)
        assert.is.same({ w1 }, layout:get_children())

        layout:add_widget_at(w2, 2, 2)
        assert.is.same({ w1, w2 }, layout:get_children())

        layout:add_widget_at(w1, 1, 2)
        assert.is.same({ w1, w2, w1 }, layout:get_children())

        layout:reset()
        assert.is.same({}, layout:get_children())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
