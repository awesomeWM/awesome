local gears_obj = require("gears.object")

local tag, meta = awesome._shim_fake_class()

local function new_tag(_, args)
    local ret = gears_obj()

    ret.data = {}
    ret.name = args.name or "test"
    ret.activated = true
    ret.selected = true

    function ret:clients(_) --TODO handle new
        local list = {}
        for _, c in ipairs(client.get()) do
            if c.screen == (ret.screen or screen[1]) then
                table.insert(list, c)
            end
        end

        return list
    end

    table.insert(root._tags, ret)

    return setmetatable(ret, {
                        __index     = function(...) return meta.__index(...) end,
                        __newindex = function(...) return meta.__newindex(...) end
                    })
end

return setmetatable(tag, {
                    __call      = new_tag,
                })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

