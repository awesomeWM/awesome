-- This script is given to Busted via the --helper argument. Modules loaded here
-- won't be cleared and reloaded by Busted. This is needed for lgi because lgi
-- is not safe to reload and yet Busted makes us try to do this.
require("lgi")
