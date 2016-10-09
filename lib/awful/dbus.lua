---------------------------------------------------------------------------
--- D-Bus module for awful.
--
-- This module simply request the org.naquadah.awesome.awful name on the D-Bus
-- for futur usage by other awful modules.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @module awful.dbus
---------------------------------------------------------------------------

-- Grab environment we need
local dbus = dbus

if dbus then
    dbus.request_name("session", "org.naquadah.awesome.awful")
end

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
