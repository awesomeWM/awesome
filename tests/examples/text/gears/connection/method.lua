--DOC_GEN_IMAGE

local gears = {object = require("gears.object"), connection = require("gears.connection")} --DOC_HIDE

local called = false --DOC_HIDE

-- When `source` changes, `target` is updated.
local my_source_object = gears.object { --DOC_HIDE
    enable_properties   = true, --DOC_HIDE
    enable_auto_signals = true  --DOC_HIDE
}--DOC_HIDE

local my_target_object = gears.object {--DOC_HIDE
    enable_properties   = true, --DOC_HIDE
    enable_auto_signals = true  --DOC_HIDE
} --DOC_HIDE

-- Declare a method.
function my_target_object:my_method()
    called = true --DOC_HIDE
    -- do something
end

my_source_object.foo = 42 --DOC_HIDE

--DOC_NEWLINE

local conn = gears.connection {
    source          = my_source_object,
    source_property = "foo",
    target          = my_target_object,
    target_method   = "my_method"
}

assert(conn) --DOC_HIDE
assert(conn.target == my_target_object) --DOC_HIDE
assert(conn.enabled) --DOC_HIDE
assert(conn.target_method == "my_method") --DOC_HIDE

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(called) --DOC_HIDE

