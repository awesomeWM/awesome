---------------------------------------------------------------------------
--- tmux hotkeys for awful.hotkeys_widget
--
-- @author nahsi nashi@airmail.cc
-- @copyright 2017 nahsi
-- @submodule  awful.hotkeys_popup
---------------------------------------------------------------------------

local hotkeys_popup = require("awful.hotkeys_popup.widget")

local tmux = {}

--- Add rules to match tmux session.
--
-- For example:
--
--     tmux.add_rules_for_terminal({ rule = { name = { "tmux" }}})
--
-- will show tmux hotkeys for any window that has 'tmux' in its title.
-- If no rules are provided then tmux hotkeys will be shown always!
-- @function add_rules_for_terminal
-- @see awful.rules.rules
-- @tparam table rule Rules to match a window containing a tmux session.
function tmux.add_rules_for_terminal(rule)
    for group_name, group_data in pairs({
        ["tmux: sessions"] = rule,
        ["tmux: windows"]  = rule,
        ["tmux: panes"]    = rule,
        ["tmux: misc"]     = rule,
    }) do
        hotkeys_popup.add_group_rules(group_name, group_data)
    end
end

local tmux_keys = {
    ["tmux: sessions"] = {{
        modifiers = {},
        keys = {
            s     = "show all sessions",
            ['$'] = "rename the current session",
            ['('] = "move to previous session",
            [')'] = "move to next session",
            d     = "detach from current session"
        }
    }},

    ["tmux: windows"] = {{
        modifiers = {},
        keys = {
            c         = "create window",
            f         = "find window",
            [',']     = "rename current window",
            --['&']     = "close current window",
            p         = "previous window",
            n         = "next window",
            ['0...9'] = "select window by number"
        }
    }},

    ["tmux: panes"] = {{
        modifiers = {},
        keys = {
            [';']       = "toggle last active pane",
            ['%']       = "split pane vertically",
            ['"']       = "split pane horizontally",
            ['{']       = "move the current pane left",
            ['}']       = "move the current pane right",
            ['q 0...9'] = "select pane by number",
            o           = "toggle between panes",
            z           = "toggle pane zoom",
            ['space']   = "toggle between layouts",
            ['!']       = "convert pane into a window",
            x           = "close current pane"
        }
    }},

    ["tmux: misc"] = {{
        modifiers = {},
        keys = {
            [':'] = "enter command mode",
            ['?'] = "list shortcuts",
            t     = "show clock"
        }
    }}
}

hotkeys_popup.add_hotkeys(tmux_keys)

return tmux

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
