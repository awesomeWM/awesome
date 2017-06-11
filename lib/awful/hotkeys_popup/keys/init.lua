---------------------------------------------------------------------------
--- Additional hotkeys for awful.hotkeys_widget
--
-- @author Yauheni Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2014-2015 Yauheni Kirylau
-- @module awful.hotkeys_popup.keys
---------------------------------------------------------------------------


local keys = {
  vim = require("awful.hotkeys_popup.keys.vim"),
  firefox = require("awful.hotkeys_popup.keys.firefox"),
  qutebrowser = require("awful.hotkeys_popup.keys.qutebrowser"),
}
return keys

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
