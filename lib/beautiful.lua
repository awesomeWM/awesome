-- Work-around for broken systems which are updated by overwriting the awesome
-- installation. This would not remove beautiful.lua from older awesome versions
-- and thus breakage follows.
-- The work-around is to use a pointless beautiful.lua file.
return require("beautiful.init")

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
