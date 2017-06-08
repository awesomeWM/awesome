---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local constraint = require("wibox.container.constraint")
local utils = require("wibox.test_utils")

describe("wibox.container.constraint", function()
    it("common interfaces", function()
        utils.test_container(constraint())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
