--DOC_GEN_OUTPUT --DOC_HIDE
local gears = require("gears") --DOC_HIDE

local test = "do.it"
local res = gears.string.startswith(test,"do")
print(tostring(res)) -- Print boolean value
assert(res == true) --DOC_HIDE

res = gears.string.startswith(test,"it")
print(tostring(res)) -- print boolean value
assert(res == false) --DOC_HIDE

res = gears.string.startswith(nil,"do")
print(tostring(res)) -- print boolean value
assert(res == false) --DOC_HIDE
