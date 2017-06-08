---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local arcchart = require("wibox.container.arcchart")
local utils = require("wibox.test_utils")

describe("wibox.container.arcchart", function()
    it("common interfaces", function()
        utils.test_container(arcchart())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
