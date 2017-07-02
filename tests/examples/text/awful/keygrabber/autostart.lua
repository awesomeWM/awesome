
local awful = { keygrabber = require("awful.keygrabber") } --DOC_HIDE

local naughty = { notification = function() end } --DOC_HIDE

local autostart_works = false --DOC_HIDE

awful.keygrabber {
    autostart      = true,
    stop_key    = "Return",
    stop_callback  = function(_, _, _, sequence)
        autostart_works = true --DOC_HIDE
        assert(sequence == "abc") --DOC_HIDE
        naughty.notification {text="The keys were:"..sequence}
    end,
}

for _, v in ipairs {"a", "b", "c"} do--DOC_HIDE
    root.fake_input("key_press"  , v)--DOC_HIDE
    root.fake_input("key_release", v)--DOC_HIDE
end--DOC_HIDE

root.fake_input("key_press"  , "Return")--DOC_HIDE
root.fake_input("key_release", "Return")--DOC_HIDE

assert(autostart_works) --DOC_HIDE
