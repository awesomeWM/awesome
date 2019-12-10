--DOC_GEN_OUTPUT --DOC_HIDE
local gears = require("gears") --DOC_HIDE

local res = {"a", "b", "c", "d", "e"}

for i=1, #res do
    local k = res[i]
    local v = gears.table.cycle_value(res, k, 1)
    print(v)
end
