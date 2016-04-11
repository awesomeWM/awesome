local gears_obj = require("gears.object")

local tag, meta = awesome._shim_fake_class()

local function new_tag(_, args)
    local ret = gears_obj()

    ret:add_signal("property::layout")
    ret:add_signal("property::name")
    ret:add_signal("property::geometry")
    ret:add_signal("property::screen")
    ret:add_signal("property::master_width_factor")
    ret:add_signal("property::mwfact")
    ret:add_signal("property::ncol")
    ret:add_signal("property::column_count")
    ret:add_signal("property::nmaster")
    ret:add_signal("property::index")
    ret:add_signal("property::useless_gap")
    ret:add_signal("property::_wa_tracker")

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

tag:_add_signal("request::select")
tag:_add_signal("property::selected")
tag:_add_signal("property::activated")
tag:_add_signal("tagged")
tag:_add_signal("untagged")

return setmetatable(tag, {
    __call      = new_tag,
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

