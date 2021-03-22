--DOC_HIDE --DOC_GEN_IMAGE --DOC_HEADER
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local awful = {--DOC_HIDE
    button = require("awful.button"), --DOC_HIDE
    widget = {watch = require("awful.widget.watch")} --DOC_HIDE
}--DOC_HIDE
local aspawn = require("awful.spawn") --DOC_HIDE
aspawn.easy_async = function(_, cb)--DOC_HIDE
    cb("Hello world!", "", nil, 0)--DOC_HIDE
end--DOC_HIDE
aspawn.with_line_callback = function() end --DOC_HIDE

    local watch = --DOC_HIDE
    awful.widget.watch('bash -c "echo Hello world! | grep Hello"', 15)

watch.forced_height = 24 --DOC_HIDE

parent:add(watch) --DOC_HIDE
