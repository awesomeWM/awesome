--DOC_GEN_OUTPUT --DOC_HIDE
local gears = require("gears") --DOC_HIDE

local test = "do it"
local res = gears.string.linewrap(test, 2, 0)
print(res)
