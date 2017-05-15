local gears_obj = require("gears.object")

local screen, meta = awesome._shim_fake_class()

screen.count = 1

local function create_screen(args)
    local s = gears_obj()
    s.data = {}
    s.valid = true

    -- Copy the geo in case the args are mutated
    local geo = {
        x      = args.x     ,
        y      = args.y     ,
        width  = args.width ,
        height = args.height,
    }

    function s._resize(args2)
        geo.x      = args2.x      or geo.x
        geo.y      = args2.y      or geo.y
        geo.width  = args2.width  or geo.width
        geo.height = args2.height or geo.height
    end

    local wa = args.workarea_sides or 10

    return setmetatable(s,{ __index = function(_, key)
        if key == "geometry" then
            return {
                x      = geo.x or 0,
                y      = geo.y or 0,
                width  = geo.width ,
                height = geo.height,
            }
        elseif key == "workarea" then
            return {
                x      = (geo.x or 0) + wa  ,
                y      = (geo.y or 0) + wa  ,
                width  = geo.width    - 2*wa,
                height = geo.height   - 2*wa,
            }
        else
            return meta.__index(_, key)
        end
    end,
    __newindex = function(...) return meta.__newindex(...) end
})
end

local screens = {}

function screen._add_screen(args)
    local s = create_screen(args)
    table.insert(screens, s)

    -- Skip the metatable or it will have side effects
    rawset(s, "index", #screens)

    screen[#screen+1] = s
    screen[s] = s
end

function screen._get_extents()
    local xmax, ymax
    for _, v in ipairs(screen) do
        if not xmax or v.geometry.x+v.geometry.width > xmax.geometry.x+xmax.geometry.width then
            xmax = v
        end
        if not ymax or v.geometry.y+v.geometry.height > ymax.geometry.y+ymax.geometry.height then
            ymax = v
        end
    end

    return xmax.geometry.x+xmax.geometry.width, ymax.geometry.y+ymax.geometry.height
end

function screen._clear()
    for i=1, #screen do
        screen[screen[i]] = nil
        screen[i] = nil
    end
    screens = {}
end

function screen._setup_grid(w, h, rows, args)
    args = args or {}
    screen._clear()
    for i, row in ipairs(rows) do
        for j=1, row do
            args.x      = (j-1)*w + (j-1)*10
            args.y      = (i-1)*h + (i-1)*10
            args.width  = w
            args.height = h
            screen._add_screen(args)
        end
    end
end

local function iter_scr(_, _, s)
    if not s then
        assert(screen[1])
        return screen[1], 1
    end

    local i = s.index

    if i +1  < #screen then
        return screen[i+1], i+1
    end
end

screen._add_screen {width=320, height=240}

return setmetatable(screen, {
    __call = iter_scr
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
