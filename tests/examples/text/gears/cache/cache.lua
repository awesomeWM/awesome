--DOC_GEN_OUTPUT --DOC_HIDE
local cache = require("gears.cache") --DOC_HIDE
local test_cache = cache.new(function(test)
    -- let's just print about what we created
    print("new entry created with value " .. test)
    -- Pretend this is some expensive computation
    return test * 42
end)
-- Populate the cache
print(test_cache:get(0))
print(test_cache:get(1))
print(test_cache:get(2))
-- no message since it exists
print(test_cache:get(0))
