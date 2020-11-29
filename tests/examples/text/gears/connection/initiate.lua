--DOC_GEN_IMAGE

local gears = {object = require("gears.object"), connection = require("gears.connection")} --DOC_HIDE

-- When `source` changes, `target` is updated.
local my_source_object = gears.object { --DOC_HIDE
    enable_properties   = true,--DOC_HIDE
    enable_auto_signals = true--DOC_HIDE
}--DOC_HIDE

local my_target_object1 = gears.object {--DOC_HIDE
    enable_properties   = true,--DOC_HIDE
    enable_auto_signals = true--DOC_HIDE
}--DOC_HIDE

local my_target_object2 = gears.object {--DOC_HIDE
    enable_properties   = true,--DOC_HIDE
    enable_auto_signals = true--DOC_HIDE
}--DOC_HIDE


my_source_object.foo = 42

--DOC_NEWLINE

local conn = --DOC_HIDE
gears.connection {
    source          = my_source_object,
    source_property = "foo",
    target          = my_target_object1,
    target_property = "bar"
}

assert(conn.source_property == "foo") --DOC_HIDE
assert(conn.source == my_source_object) --DOC_HIDE
assert(conn.target == my_target_object1) --DOC_HIDE
assert(conn.target_property == "bar") --DOC_HIDE
assert(conn.initiate) --DOC_HIDE

--DOC_NEWLINE

conn = --DOC_HIDE
gears.connection {
    source          = my_source_object,
    source_property = "foo",
    initiate        = false,
    target          = my_target_object2,
    target_property = "bar"
}

--DOC_NEWLINE

assert(not conn.initiate) --DOC_HIDE

-- my_target_object1 should be initialized, but not my_target_object2.
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(my_target_object1.bar == 42)
assert(my_target_object2.bar == nil)

conn.sources = {my_source_object, my_target_object1} --DOC_HIDE
