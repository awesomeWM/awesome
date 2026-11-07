---------------------------------------------------------------------------
--- fish hotkeys for awful.hotkeys_popup.widget
--
-- @author Yauheni Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2014-2015 Yauheni Kirylau
-- @submodule  awful.hotkeys_popup
---------------------------------------------------------------------------

local hotkeys_popup = require("awful.hotkeys_popup.widget")

local fish = {}

--- Add rules to match fish shell.
--
-- For example:
--
--     fish.add_rules_for_terminal({ rule = { name = "fish" }})
--
-- will show fish hotkeys for any window that has 'fish' in its title.
-- If no rules are provided then fish hotkeys won't be shown.
-- @function add_rules_for_terminal
-- @see awful.rules.rules
-- @tparam table rule Rules to match a window containing a fish shell.
function fish.add_rules_for_terminal(rule)
    for group_name, group_data in pairs({
        ["fish"] = rule,
    }) do
        hotkeys_popup.add_group_rules(group_name, group_data)
    end
end

-- don't show hotkeys by default
fish.add_rules_for_terminal({rule={focusable=false}})

local fish_keys = {

    ["fish"] = {{
        modifiers = {},
        keys = {
            Next="history begin",
            Prior="history end",
            Tab="complete",
        }
    }, {
        modifiers = {"Ctrl"},
        keys = {
            a="BOL",
            b="one char back",
            c="cancel",
            d="delete/exit",
            e="EOL",
            f="one char forward",
            k="kill from cursor to EOL",
            l="clear screen",
            n="history next",
            p="history prev",
            t="transpose char with prev",
            u="kill from BOL to cursor",
            v="paste from clipboard",
            w="kill prev word",
            x="copy buffer to clipboard",
            y="yank from killring",
            z="send sigstop",
        }
    }, {
        modifiers = {"Alt"},
        keys = {
            Enter="\\n",
            Left="directory/one word back",
            Right="directory/one word forward",
            Up="search token in history",
            Down="search token back in history",
            b="one word back",
            c="capitalize word",
            d="kill next word",
            e="edit line in vim",
            f="one word forward",
            h="show man",
            l="ls",
            p="| less;",
            t="transpose word with prev",
            u="uppercase word",
            v="edit in $VISUAL or $EDITOR",
            w="print short help",
            y="pop killring value",
            ["."]="search token in history",

        }
    }, {
        modifiers = {"Alt", "Shift"},
        keys = {
            [","]="beginning of buffer",
            ["."]="end of buffer",
        },
    }, {
        modifiers = {"Shift"},
        keys = {
            Tab="complete and fuzzy search",
        },
    }},
}

hotkeys_popup.add_hotkeys(fish_keys)

return fish

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
