local gobject = require("gears.object")
local gtable  = require("gears.table")

return setmetatable({_is_capi_key = true}, {__call = function(_, args)
    return gtable.crush(gobject(), args)
end})
