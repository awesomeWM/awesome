
local awful = { keygrabber = require("awful.keygrabber") } --DOC_HIDE

local allowed_keys_works = false --DOC_HIDE

awful.keygrabber {
    autostart      = true,
    allowed_keys   = {"a", "w", "e", "s", "o", "m", "e"},
    stop_callback  = function() --DOC_HIDE
        allowed_keys_works = true --DOC_HIDE
    end, --DOC_HIDE
}

root.fake_input("key_press"  , "w")--DOC_HIDE
root.fake_input("key_release", "w")--DOC_HIDE

assert(not allowed_keys_works) --DOC_HIDE

root.fake_input("key_press"  , "q")--DOC_HIDE
root.fake_input("key_release", "q")--DOC_HIDE

assert(allowed_keys_works) --DOC_HIDE
