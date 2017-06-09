---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local rotate = require("wibox.container.rotate")
local utils = require("wibox.test_utils")

describe("wibox.container.rotate", function()
    it("common interfaces", function()
        utils.test_container(rotate())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
