-- Work-around for broken systems which are updated by overwriting the awesome
-- installation. This would not remove naughty.lua from older awesome versions
-- and thus breakage follows.
-- The work-around is to use a pointless naughty.lua file.
return require("naughty.init")

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
