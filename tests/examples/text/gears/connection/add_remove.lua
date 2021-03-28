--DOC_GEN_IMAGE

local gears = {object = require("gears.object"), connection = require("gears.connection")} --DOC_HIDE

-- When `source` changes, `target` is updated.
local my_source_object1 = gears.object { --DOC_HIDE
    enable_properties   = true,--DOC_HIDE
    enable_auto_signals = true--DOC_HIDE
}--DOC_HIDE

local my_source_object2 = gears.object {--DOC_HIDE
    enable_properties   = true,--DOC_HIDE
    enable_auto_signals = true--DOC_HIDE
}--DOC_HIDE

local my_target_object1 = gears.object {--DOC_HIDE
    enable_properties   = true,--DOC_HIDE
    enable_auto_signals = true--DOC_HIDE
}--DOC_HIDE

local conn = gears.connection {
    source = my_source_object1,
    target = my_target_object1,
}

--DOC_NEWLINE

conn:append_source_object(my_source_object1)
--DOC_NEWLINE
assert(conn:has_source_object(my_source_object1))
--DOC_NEWLINE
assert(not conn:has_source_object(my_source_object2)) --DOC_HIDE
conn:append_source_object(my_source_object2)
assert(conn:has_source_object(my_source_object2)) --DOC_HIDE
local ret = --DOC_HIDE
conn:remove_source_object(my_source_object1)
assert(ret) --DOC_HIDE
assert(not conn:has_source_object(my_source_object1)) --DOC_HIDE
assert(not  conn:remove_source_object({})) --DOC_HIDE
