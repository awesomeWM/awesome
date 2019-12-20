--DOC_GEN_OUTPUT --DOC_HIDE
local gears = require("gears") --DOC_HIDE

local test = "do.t"
local res = gears.string.linecount(test)
print("Count is: " .. res)

local test2 = "do\nit\nnow"
local res2 = gears.string.linecount(test2)
print("Count is: " .. res2)

