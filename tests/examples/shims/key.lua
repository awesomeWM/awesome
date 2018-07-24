local gobject = require("gears.object")
local gtable  = require("gears.table")

return setmetatable({}, {__call = function(_, args)
    return gtable.crush(gobject(), args)
end})
