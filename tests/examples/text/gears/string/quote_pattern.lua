--DOC_GEN_OUTPUT --DOC_HIDE
local gears = require("gears") --DOC_HIDE

local test = "do.it"
local res = gears.string.quote_pattern(test)
print(res)
