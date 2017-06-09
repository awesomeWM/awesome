---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
---------------------------------------------------------------------------

package.loaded["gears.timer"] = {}

local scroll = require("wibox.container.scroll")
local utils = require("wibox.test_utils")

describe("wibox.container.scroll", function()
    it("common interfaces", function()
        utils.test_container(scroll.horizontal())
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
