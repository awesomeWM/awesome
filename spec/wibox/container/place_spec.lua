---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

local place = require("wibox.container.place")
local utils = require("wibox.test_utils")

describe("wibox.container.place", function()
    it("common interfaces", function()
        utils.test_container(place())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
