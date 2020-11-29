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

local conn1 = gears.connection {
    source          = my_source_object,
    source_property = "foo",
    target          = my_target_object1,
    target_property = "bar"
}

--DOC_NEWLINE

local conn2 = gears.connection {
    source          = my_source_object,
    source_property = "foo",
    target          = my_target_object2,
    target_property = "bar"
}

--DOC_NEWLINE

conn1.enabled = true
conn2.enabled = false

--DOC_NEWLINE

-- conn1 should be enabled, but not conn2.
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(my_target_object1.bar == 42)
assert(my_target_object2.bar == nil)

