---------------------------------------------------------------------------
--- Additional hotkeys for awful.hotkeys_widget
--
-- @author Yauheni Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2014-2015 Yauheni Kirylau
-- @submodule awful.hotkeys_popup
---------------------------------------------------------------------------


local keys = {
  vim = require("awful.hotkeys_popup.keys.vim"),
  firefox = require("awful.hotkeys_popup.keys.firefox"),
  tmux = require("awful.hotkeys_popup.keys.tmux"),
  qutebrowser = require("awful.hotkeys_popup.keys.qutebrowser"),
  termite = require("awful.hotkeys_popup.keys.termite"),
  fish = require("awful.hotkeys_popup.keys.fish"),
}


--- Add rules to match terminal.
--
-- For example:
--
--     keys.add_rules_for_terminal({ rule = { class = "XTerm" }})
--
-- will show hotkeys for `fish` and `tmux` if `xterm` terminal is running.
-- @function add_rules_for_terminal
-- @see awful.rules.rules
-- @tparam table rule Rules to match a terminal window.
function keys.add_rules_for_terminal(rule)
    keys.tmux.add_rules_for_terminal(rule)
    keys.fish.add_rules_for_terminal(rule)
end


return keys

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
