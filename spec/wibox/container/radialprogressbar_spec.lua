---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local radialprogressbar = require("wibox.container.radialprogressbar")
local utils = require("wibox.test_utils")

describe("wibox.container.radialprogressbar", function()
    it("common interfaces", function()
        utils.test_container(radialprogressbar())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
