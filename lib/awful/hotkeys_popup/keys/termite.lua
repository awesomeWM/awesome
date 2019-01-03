---------------------------------------------------------------------------
--- Termite hotkeys for awful.hotkeys_widget
--
-- @author ikselven
-- @copyright 2017 ikselven
-- @submodule awful.hotkeys_popup
---------------------------------------------------------------------------

local hotkeys_popup = require("awful.hotkeys_popup.widget")

local termite_rule = { class = "Termite" }
for group_name, group_data in pairs({
    ["termite: Insert Mode"] = { color = "#35A91E", rule = termite_rule },
    ["termite: Selection Mode"] = { color = "#35A91E", rule = termite_rule },
}) do
    hotkeys_popup.add_group_rules(group_name, group_data)
end

local termite_keys = {

    ["termite: Insert Mode"] = {{
        modifiers = { "Ctrl" },
        keys = {
            Tab = "start scrollback completion",
            ["+"] = "increase font size",
            ["-"] = "decrease font size",
            ["="] = "reset font size to default"
        }
    }, {
        modifiers = { "Shift" },
        keys = {
            PageUp = "scroll up a page",
            PageDown = "scroll down a page"
        }
    },{
        modifiers = { "Ctrl", "Shift" },
        keys = {
            x = "activate url hints mode",
            r = "reload configuration file",
            c = "copy to CLIPBOARD",
            v = "paste from CLIPBOARD",
            u = "unicode input (standard GTK binding)",
            Space = "enter selection mode",
            t = "open a new terminal in the current directory",
            Up = "scroll up a line",
            Down = "scroll down a line"
        }
    }},

    ["termite: Selection Mode"] = {{
        modifiers = {},
        keys = {
            q = "enter insert mode",
            Esc = "enter insert mode",
            x = "activate url hints mode",
            v = "visual mode",
            hjkl = "move cursor left/down/up/right",
            w = "forward word",
            e = "forward to end of word",
            b = "backward word",
            ["0"] = "move cursor to first column in the row",
            Home = "move cursor to first column in the row",
            ["^"] = "beginning-of-line (first non-blank character)",
            ["$"] = "end-of-line",
            End = "end-of-line",
            g = "jump to start of first row",
            y = "copy to CLIPBOARD",
            ["/"] = "forward search",
            ["?"] = "reverse search",
            u = "forward url search",
            o = "open the current selection as a url",
            Return = "open the current selection as a url and enter insert mode",
            n = "next search match"
        }
    }, {
        modifiers = { "Shift" },
        keys = {
            v = "visual line mode",
            Right = "forward word",
            Left = "backward word",
            w = "forward WORD (non-whitespace)",
            e = "forward to end of WORD (non-whitespace)",
            b = "backward WORD (non-whitespace)",
            h = "jump to the top of the screen",
            m = "jump to the middle of the screen",
            l = "jump to the bottom of the screen",
            g = "jump to start of last row",
            u = "reverse url search",
            n = "previous search match"
        }
    }, {
        modifiers = { "Ctrl" },
        keys = {
            ["["] = "enter insert mode",
            v = "visual block mode",
            Right = "forward WORD (non-whitespace)",
            Left = "backward WORD (non-whitespace)",
            u = "move cursor a half screen up",
            d = "move cursor a half screen down",
            b = "move cursor a full screen up (back)",
            f = "move cursor a full screen down (forward)"
        }
    }}
}

hotkeys_popup.add_hotkeys(termite_keys)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
