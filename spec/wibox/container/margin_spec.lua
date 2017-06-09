---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local margin = require("wibox.container.margin")
local utils = require("wibox.test_utils")

describe("wibox.container.margin", function()
    it("common interfaces", function()
        utils.test_container(margin())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
