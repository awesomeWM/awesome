--DOC_GEN_IMAGE
local gears = { watcher = require("gears.watcher") } --DOC_HIDE

function gears.watcher._easy_async(_, callback)  --DOC_HIDE
    callback("three")  --DOC_HIDE
end --DOC_HIDE

local watcher = gears.watcher {
    interval = 5,
    command  = "echo one:two:three | cut -f3 -d :",
    shell    = true,
}

assert(watcher) --DOC_HIDE
