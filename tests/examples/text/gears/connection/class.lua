--DOC_GEN_IMAGE

local gears = {object = require("gears.object"), connection = require("gears.connection")} --DOC_HIDE

client.gen_fake {name = "foo"} --DOC_HIDE
client.gen_fake {name = "baz"} --DOC_HIDE

local called = 0 --DOC_HIDE

--DOC_NEWLINE

local conn = gears.connection {
    source_class = client,
    signals      = {"focused", "property::name"},
    initiate     = false,
    callback     = function()
        called = called + 1 --DOC_HIDE
        -- do something
    end
}

assert(conn) --DOC_HIDE
assert(conn.signals[1] == "focused") --DOC_HIDE
assert(conn.signals[2] == "property::name") --DOC_HIDE
assert(conn.source_class == client) --DOC_HIDE
assert(conn.callback) --DOC_HIDE

--DOC_NEWLINE

-- This emit the `focused` signal.
screen[1].clients[1]:activate{}
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(called == 1) --DOC_HIDE

--DOC_NEWLINE

-- Changing the name emits `property::name`.
screen[1].clients[1].name = "bar"
client.emit_signal("property::name", screen[1].clients[1]) --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(called == 2) --DOC_HIDE

