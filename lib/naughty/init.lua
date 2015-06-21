---------------------------------------------------------------------------
-- @author Uli Schlachter &lt;psychon@znc.in&gt;
-- @copyright 2014 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @module naughty
---------------------------------------------------------------------------

local naughty = require("naughty.core")
if dbus then
    naughty.dbus = require("naughty.dbus")
end

return naughty

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
