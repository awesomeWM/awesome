---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @module gears
---------------------------------------------------------------------------

local require_luarocks = require("gears.require_luarocks")
require_luarocks.require("lgi")

-- Keep this in sync with build-utils/lgi-check.sh and .travis.yml (LGIVER)!
local ver_major, ver_minor, ver_patch = string.match(require_luarocks.require('lgi.version'), '(%d)%.(%d)%.(%d)')
if tonumber(ver_major) <= 0 and (tonumber(ver_minor) < 8 or (tonumber(ver_minor) == 8 and tonumber(ver_patch) < 0)) then
    error("lgi too old, need at least version 0.8.0")
end

return
{
    color = require("gears.color");
    debug = require("gears.debug");
    object = require("gears.object");
    surface = require("gears.surface");
    wallpaper = require("gears.wallpaper");
    timer = require("gears.timer");
    cache = require("gears.cache");
    matrix = require("gears.matrix");
    shape = require("gears.shape");
    protected_call = require("gears.protected_call");
    geometry = require("gears.geometry");
    math = require("gears.math");
    table = require("gears.table");
    string = require("gears.string");
    filesystem = require("gears.filesystem");
    require_luarocks = require_luarocks;
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
