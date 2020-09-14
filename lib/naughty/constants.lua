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
local gtable = require("gears.table")
local dpi = beautiful.xresources.apply_dpi

local ret, no_clear = {}, {}

ret.config = {
    padding         = dpi(4),
    spacing         = dpi(1),
    icon_dirs       = { "/usr/share/pixmaps/", "/usr/share/icons/hicolor" },
    icon_formats    = { "png", "gif" },
    notify_callback = nil,
}

no_clear.presets = {
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
    {{urgency = ret.config._urgency.low     }, no_clear.presets.low}, --compat
    {{urgency = ret.config._urgency.normal  }, no_clear.presets.normal}, --compat
    {{urgency = ret.config._urgency.critical}, no_clear.presets.critical}, --compat
    {{urgency = "low"     }, no_clear.presets.low},
    {{urgency = "normal"  }, no_clear.presets.normal},
    {{urgency = "critical"}, no_clear.presets.critical},
}

no_clear.defaults = {
    timeout      = 5,
    text         = "",
    screen       = nil,
    ontop        = true,
    margin       = dpi(5),
    border_width = dpi(1),
    position     = "top_right",
    urgency      = "normal",
    message      = "",
    title        = "",
    app_name     = "",
    ignore       = false,
}

ret.notification_closed_reason = {
    too_many_on_screen   = -2,
    silent               = -1,
    expired              = 1,
    dismissedByUser      = 2, --TODO v5 remove this undocumented legacy constant
    dismissed_by_user    = 2,
    dismissedByCommand   = 3, --TODO v5 remove this undocumented legacy constant
    dismissed_by_command = 3,
    undefined            = 4
}

-- Legacy --TODO v5 remove this alias
ret.notificationClosedReason = ret.notification_closed_reason

-- `no_clear` is used to prevent users from setting the entire table.
-- If they did and we added a new default value, then it would not be
-- backward compatible. For safety, we just crush the tables rather than
-- replace them.
setmetatable(ret.config, {
    __index = function(_, key)
        return no_clear[key]
    end,
    __newindex = function(_, key, value)
        if no_clear[key] then
            gtable.crush(no_clear[key], value)
        else
            rawset(ret.config, key, value)
        end
    end
})

return ret
