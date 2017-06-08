---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local mirror = require("wibox.container.mirror")
local utils = require("wibox.test_utils")

describe("wibox.container.mirror", function()
    it("common interfaces", function()
        utils.test_container(mirror())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
