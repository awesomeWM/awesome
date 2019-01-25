-- This script is given to Busted via the --helper argument. Modules loaded here
-- won't be cleared and reloaded by Busted. This is needed for lgi because lgi
-- is not safe to reload and yet Busted manages to do this.
require("lgi")

-- Always show deprecated messages
_G.awesome = {version = "v9999"}

-- "fix" some intentional beautiful breakage done by .travis.yml
require("beautiful").init{a_key="a_value"}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
