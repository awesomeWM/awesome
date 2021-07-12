---------------------------------------------------------------------------
--- Kitty hotkeys for awful.hotkeys_widget
--
-- @author Samuel Abreu
-- @copyright 2021 Samuel Abreu
-- @submodule awful.hotkeys_popup
---------------------------------------------------------------------------
local hotkeys_popup = require("awful.hotkeys_popup.widget")

local kitty_rule = { class = { "kitty", "Kitty" } }
for group_name, group_data in pairs({
    ["Kitty"] = { color = "#35A91E", rule_any = kitty_rule },
    ["Kitty: Hints"] = { color = "#35A91E", rule_any = kitty_rule },
}) do
    hotkeys_popup.add_group_rules(group_name, group_data)
end

local kitty_keys = {
    ["Kitty"] = {{
        modifiers = { "Ctrl", "Shift" },
        keys = {
            Enter = "Split new Terminal",
            l = "Next layout",
            t = "New tab",
            q = "Close tab",
            Right = "Next tab",
            Left = "Previous tab",
            n = "New OS Window",
            ["]"] = "Next Window",
            ["["] = "Previous Window",
            ["0..9"] = "Focus specific window"
        }
    }},
    ["Kitty: Hints"] = {{
        modifiers = { "Ctrl", "Shift" },
        keys = {
            e = "Choose URL to Open",
        }
    },
    {
        modifiers = { "Ctrl", "Shift", "p" },
        keys = {
            f = "Choose path to paste",
            n = "Choose filename:n to open"
        }
    }}
}

hotkeys_popup.add_hotkeys(kitty_keys)
