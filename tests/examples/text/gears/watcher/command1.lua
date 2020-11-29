--DOC_NO_USAGE --DOC_GEN_OUTPUT
local gears = { watcher = require("gears.watcher") } --DOC_HIDE

function gears.watcher._easy_async(_, cb) --DOC_HIDE
    cb("one two three", "", nil, 0) --DOC_HIDE
end --DOC_HIDE

    local w = gears.watcher {
        interval = 0.05,
        command  = "echo one two three",
    }

    --DOC_NEWLINE
    -- [wait some time]

    --DOC_NEWLINE
    -- It will be "one two three"
    print(w.value)
