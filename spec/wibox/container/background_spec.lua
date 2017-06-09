---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local background = require("wibox.container.background")
local utils = require("wibox.test_utils")

describe("wibox.container.background", function()
    it("common interfaces", function()
        utils.test_container(background())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
