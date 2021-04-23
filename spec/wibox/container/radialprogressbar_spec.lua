---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local radialprogressbar = require("wibox.container.radialprogressbar")
local utils = require("wibox.test_utils")
local base = require("wibox.widget.base")

describe("wibox.container.radialprogressbar", function()
    it("common interfaces", function()
        utils.test_container(radialprogressbar())
    end)

    describe("constructor", function()
        it("applies arguments", function()
            local inner = base.empty_widget()
            local widget = radialprogressbar(inner)

            assert.is.same(widget:get_children(), { inner })
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
