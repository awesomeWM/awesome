local gdebug = require("gears.debug")

-- It diverged over time to the point where it had nothing to do with EWMH.
return gdebug.deprecate_class(
    require("awful.permissions"),
    "awful.ewmh",
    "awful.permissions",
    { deprecated_in = 5}
)
