local rawget     = rawget

globalwidgets = {
    left = {},
    middle = {},
    right = {},
    byname = {}
}
local function wrequire(table, key)
    local module = rawget(table, key)
    return module or require(table._NAME .. '.' .. key)
end
return setmetatable({}, {__index = wrequire, __call = function(_,...) return worker(...) end})
