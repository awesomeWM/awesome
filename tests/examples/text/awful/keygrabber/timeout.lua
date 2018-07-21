
local awful = { keygrabber = require("awful.keygrabber") } --DOC_HIDE

local timeout_works = false --DOC_HIDE

local function print() end --DOC_HIDE be silent

local g = --DOC_HIDE

awful.keygrabber {
    autostart      = true,
    timeout        = 1, -- second
    timeout_callback  = function()
        print("The keygrabber has expired")
        timeout_works = true --DOC_HIDE
    end,
}

assert(g) --DOC_HIDE Returning the object works
assert(g._private.timer)--DOC_HIDE The timer has been created
assert(g._private.timer.started) --DOC_HIDE The timer is started
assert(not timeout_works) --DOC_HIDE The callback isn't called by accident
g._private.timer:emit_signal("timeout") --DOC_HIDE
assert(timeout_works) --DOC_HIDE The callback has been called
