local beautiful = require("beautiful")
local runner = require("_runner")

local t = screen.primary.tags[1]
assert(t)

-- The default should be true
assert(t.gap_single_client == true, tostring(t.gap_single_client))

-- Beautiful should override the default
beautiful.gap_single_client = false
assert(t.gap_single_client == false, tostring(t.gap_single_client))

-- Tag-specific properties should override beautiful
t.gap_single_client = true
assert(t.gap_single_client == true, tostring(t.gap_single_client))

runner.run_steps({ function() return true end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
