local gears_obj = require("gears.object")

local button, meta = awesome._shim_fake_class()

local function new_button(_, args)
    local ret = gears_obj()
    ret._private = args or {}

    -- The miss handler wont work for this.
    for k, v in pairs(ret._private) do
        rawset(ret, k, v)
    end

    if not rawget(ret, "modifiers") then
        rawset(ret, "modifiers", {})
    end

    --TODO v5: remove this.
    ret.data = ret._private

    rawset(ret, "_is_capi_button", true)

    local md = setmetatable(ret, {
                        __index     = function(...) return meta.__index(...) end,
                        __newindex = function(...) return meta.__newindex(...) end
                    })

    assert((not args) or args.button == md.button)

    return md
end

return setmetatable(button, { __call = new_button, })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
