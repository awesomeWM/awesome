----------------------------------------------------------------------------
--- This file hosts the shared constants used by the notification subsystem.
--
-- [[documented in core.lua]]
--
-- @author koniu &lt;gkusnierz@gmail.com&gt;
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2008 koniu
-- @copyright 2017 Emmanuel Lepage Vallee
----------------------------------------------------------------------------
local beautiful = require("beautiful")
local dpi = beautiful.xresources.apply_dpi

local ret = {}

ret.config = {
    padding         = dpi(4),
    spacing         = dpi(1),
    icon_dirs       = { "/usr/share/pixmaps/", "/usr/share/icons/hicolor" },
    icon_formats    = { "png", "gif" },
    notify_callback = nil,
}

ret.config.presets = {
    low = {
        timeout = 5
    },
    normal = {},
    critical = {
        bg      = "#ff0000",
        fg      = "#ffffff",
        timeout = 0,
    },
    ok = {
        bg      = "#00bb00",
        fg      = "#ffffff",
        timeout = 5,
    },
    info = {
        bg      = "#0000ff",
        fg      = "#ffffff",
        timeout = 5,
    },
    warn = {
        bg      = "#ffaa00",
        fg      = "#000000",
        timeout = 10,
    },
}

ret.config._urgency = {
    low      = "\0",
    normal   = "\1",
    critical = "\2"
}

ret.config.mapping = {
    {{urgency = ret.config._urgency.low     }, ret.config.presets.low}, --compat
    {{urgency = ret.config._urgency.normal  }, ret.config.presets.normal}, --compat
    {{urgency = ret.config._urgency.critical}, ret.config.presets.critical}, --compat
    {{urgency = "low"     }, ret.config.presets.low},
    {{urgency = "normal"  }, ret.config.presets.normal},
    {{urgency = "critical"}, ret.config.presets.critical},
}

ret.config.defaults = {
    timeout      = 5,
    text         = "",
    screen       = nil,
    ontop        = true,
    margin       = dpi(5),
    border_width = dpi(1),
    position     = "top_right",
    urgency      = "normal",
}

ret.notification_closed_reason = {
    too_many_on_screen   = -2,
    silent               = -1,
    expired              = 1,
    dismissedByUser      = 2, --TODO v5 remove this undocumented legacy constant
    dismissed_by_user    = 2,
    dismissedByCommand   = 3, --TODO v5 remove this undocumented legacy constant
    dismissed_by_vommand = 3,
    undefined            = 4
}

-- Legacy --TODO v5 remove this alias
ret.notificationClosedReason = ret.notification_closed_reason

return ret
