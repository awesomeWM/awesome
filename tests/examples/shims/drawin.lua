local gears_obj = require("gears.object")

local drawin, meta = awesome._shim_fake_class()
local drawins = setmetatable({}, {__mode="v"})
local cairo = require("lgi").cairo

local function new_drawin(_, args)
    local ret = gears_obj()
    ret.data = {drawable = gears_obj()}

    ret.x=0
    ret.y=0
    ret.width=1
    ret.height=1
    ret.border_width=0
    ret.ontop = false
    ret.below = false
    ret.above = false

    ret.geometry = function(_, new)
        new = new or {}
        ret.x      = new.x      or ret.x
        ret.y      = new.y      or ret.y
        ret.width  = new.width  or ret.width
        ret.height = new.height or ret.height
        return {
            x      = ret.x,
            y      = ret.y,
            width  = ret.width,
            height = ret.height
        }
    end

    ret.data.drawable.valid    = true
    ret.data.drawable.surface  = cairo.ImageSurface(cairo.Format.ARGB32, 0, 0)
    ret.data.drawable.geometry = ret.geometry
    ret.data.drawable.refresh  = function() end

    for _, k in pairs{ "buttons", "struts", "get_xproperty", "set_xproperty" } do
        ret[k] = function() end
    end

    local md = setmetatable(ret, {
                        __index     = function(...) return meta.__index(...) end,
                        __newindex = function(...) return meta.__newindex(...) end
                    })

    for k, v in pairs(args) do
        ret[k] = v
    end

    table.insert(drawins, md)

    return md
end

function drawin.get()
    return drawins
end

return setmetatable(drawin, {
                    __call      = new_drawin,
                })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
