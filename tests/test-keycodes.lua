local naughty = require("naughty")
local awful = require("awful")

local function prettyerror(key, keysym, character, expected_keysym, expected_character)
    if not keysym then keysym = "[nil]" end
    if not character then character = "nil" end
    if not expected_keysym then expected_keysym = "[nil]" end
    if not expected_character then expected_character = "nil" end
    local message = "key \""..key.."\" returned "..keysym.." ("..character..")."..
        "instead of keysym "..expected_keysym.." ("..expected_character..")"
    return string.gsub(message, "(.)", {
        ["\a"] = "\\a",
        ["\b"] = "\\b",
        ["\27"] = "\\e",
        ["\f"] = "\\f",
        ["\r"] = "\\r",
        ["\n"] = "\\n",
        ["\t"] = "\\t",
        ["\v"] = "\\v",
    })
end

local function keycode_test(key, expected_keysym, expected_character)
    local keysym, character = awful.keyboard.get_key_name(key)
    naughty.notification {
        urgency = "low",
        title   = keysym or "[nil]",
        message = character or "[nil]"
    }
    return assert(keysym == expected_keysym and character == expected_character,
        prettyerror(key, keysym, character, expected_keysym, expected_character))
end

local function triple_test(keycode, expected_keysym, expected_character)
    local result = keycode_test(keycode, expected_keysym, expected_character)
    if expected_keysym then
        local local_result = keycode_test(expected_keysym, expected_keysym, expected_character)
        result = result and local_result
    end
    if expected_character then
        local local_result = keycode_test(expected_character, expected_keysym, expected_character)
        result = result and local_result
    end
    return result
end

require("_runner").run_steps({
    function() return triple_test("#9", "Escape", "\27") end,
    function() return triple_test("#22", "BackSpace", "\b") end,
    function() return triple_test("#23", "Tab", "\t") end,
    function() return triple_test("#111", "Up", nil) end,
    function() return triple_test("#113", "Left", nil) end,
    function() return triple_test("#114", "Right", nil) end,
    function() return triple_test("#116", "Down", nil) end,
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
