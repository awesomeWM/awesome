---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local place = require("wibox.container.place")
local utils = require("wibox.test_utils")
local base = require("wibox.widget.base")

describe("wibox.container.place", function()
    it("common interfaces", function()
        utils.test_container(place())
    end)

    describe("constructor", function()
        it("applies arguments", function()
            local inner = base.empty_widget()
            local widget = place(inner)

            assert.is.equal(widget.widget, inner)
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
