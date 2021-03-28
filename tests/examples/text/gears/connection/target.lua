--DOC_GEN_IMAGE

local gears = {object = require("gears.object"), connection = require("gears.connection")} --DOC_HIDE

-- When `source` changes, `target` is updated.
local my_source_object = gears.object { --DOC_HIDE
    enable_properties   = true,--DOC_HIDE
    enable_auto_signals = true--DOC_HIDE
}--DOC_HIDE

local my_target_object = gears.object {--DOC_HIDE
    enable_properties   = true,--DOC_HIDE
    enable_auto_signals = true--DOC_HIDE
}--DOC_HIDE

my_source_object.foo = 42

--DOC_NEWLINE

local conn = gears.connection {
    source          = my_source_object,
    source_property = "foo",
    target          = my_target_object,
    target_property = "bar"
}

assert(conn) --DOC_HIDE

--DOC_NEWLINE

-- This works because `initiate` is `true` by default.
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(my_target_object.bar == 42)

--DOC_NEWLINE

-- This works because the `source` object `foo` is connected to
-- the `target` object `bar` property.
my_source_object.foo = 1337
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(my_target_object.bar == 1337)

