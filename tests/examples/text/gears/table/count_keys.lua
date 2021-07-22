--DOC_GEN_OUTPUT --DOC_HIDE
local gears = require("gears") --DOC_HIDE

local tab = { 1, nil, "a", "b", foo = "bar" }
local count = gears.table.count_keys(tab)
print("The table has " .. count .. " keys")
