---------------------------------------------------------------------------
-- This module used to be a "require only" module which, when explicitly
-- required, would allow handle focus when switching tags and other useful
-- corner cases. This code has been migrated to a more standard request::
-- API. The content itself is now in `awful.permissions`. This was required
-- to preserve backward compatibility since this module may or may not have
-- been loaded.
---------------------------------------------------------------------------
require("awful.permissions._common")._deprecated_autofocus_in_use()

if awesome.api_level > 4 then
    --TODO v5: Remove `require("awful.autofocus")` from `rc.lua`.
    require("gears.debug").deprecate(
        "The `awful.autofocus` module is deprecated, remove the require() and "..
        "look at the new `rc.lua` granted permission section in the client rules",
        {deprecated_in=5}
    )
end
