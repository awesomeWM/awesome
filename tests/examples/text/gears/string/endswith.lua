--DOC_GEN_OUTPUT --DOC_HIDE
local gears = require("gears") --DOC_HIDE

local test = "do.it"
local res = gears.string.endswith(test,"it")
print(tostring(res))
assert(res == true) --DOC_HIDE

res = gears.string.endswith(test,"do")
print(tostring(res))
assert(res == false) --DOC_HIDE

res = gears.string.endswith(nil,"it")
print(tostring(res))
assert(res == false) --DOC_HIDE
