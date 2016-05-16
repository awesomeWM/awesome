---------------------------------------------------------------------------
--- This module is deprecated and has been renamed `awful.wibar`
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.wibox
---------------------------------------------------------------------------
local util = require("awful.util")
local wibar = require("awful.wibar")

local function call(_,...)
    util.deprecate("awful.wibox has been renamed to awful.wibar")

    return wibar(...)
end

local function index(_, k)
    util.deprecate("awful.wibox has been renamed to awful.wibar")

    return wibar[k]
end

local function newindex(_, k, v)
    util.deprecate("awful.wibox has been renamed to awful.wibar")

    wibar[k] = v
end

return setmetatable({}, {__call = call, __index = index, __newindex  = newindex})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
