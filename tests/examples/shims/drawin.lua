local gears_obj = require("gears.object")

local drawin, meta = awesome._shim_fake_class()

local function new_drawin(_, args)
    local ret = gears_obj()
    ret.data = {drawable = gears_obj()}

    for k, v in pairs(args) do
        rawset(ret, k, v)
    end

    return setmetatable(ret, {
                        __index     = function(...) return meta.__index(...) end,
                        __newindex = function(...) return meta.__newindex(...) end
                    })
end

return setmetatable(drawin, {
                    __call      = new_drawin,
                })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
