local gears_obj = require("gears.object")
local gears_tab = require("gears.table")

local screen, meta = awesome._shim_fake_class()
screen._count, screen._deleted = 0, {}

local function create_screen(args)
    local s = gears_obj()
    awesome._forward_class(s, screen)

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
        local old  = gears_tab.clone(s.geometry)
        geo.x      = args2.x      or geo.x
        geo.y      = args2.y      or geo.y
        geo.width  = args2.width  or geo.width
        geo.height = args2.height or geo.height
        s:emit_signal("property::geometry", old)
    end

    s.outputs = { ["LVDS1"] = {
        mm_width  = 0,
        mm_height = 0,
    }}

    local wa = args.workarea_sides or 10

    -- This will happen if `clear()` is called
    if mouse and (screen.count() > 0 and not mouse.screen) then
        screen[s] = s
        mouse.screen = s
    end

    return setmetatable(s,{ __index = function(_, key)
            assert(s.valid)

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
        __newindex = function(...) assert(s.valid); return meta.__newindex(...) end
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
    screen._count = screen._count + 1
end

function screen._get_extents()
    local xmax, ymax
    for v in screen do
        if not xmax or v.geometry.x+v.geometry.width > xmax.geometry.x+xmax.geometry.width then
            xmax = v
        end
        if not ymax or v.geometry.y+v.geometry.height > ymax.geometry.y+ymax.geometry.height then
            ymax = v
        end
    end

    return xmax.geometry.x+xmax.geometry.width, ymax.geometry.y+ymax.geometry.height
end

-- The way the `screen` module is used to store both number and object is
-- problematic since it can cause invalid screens to be "resurrected".
local function catch_invalid(_, s)
    -- The CAPI implementation allows `nil`
    if s == nil or type(s) == "string" then return end

    assert(screen ~= s, "Some code tried (and failed) to shadow the global `screen`")

    -- Valid numbers wont get this far.
    assert(type(s) == "table", "Expected a table, but got a "..type(s))

    if type(s) == "number" then return end

    assert(s.geometry, "The object is not a screen")
    assert(s.outputs, "The object is not a screen")

    assert((not screen._deleted[s]) or (not s.valid), "The shims are broken")

    assert(not screen._deleted[s], "The screen "..tostring(s).."has been deleted")

    -- Other errors. If someone place an object in the `screen` module, it wont
    -- get this far. So the remaining cases are probably bugs.
    assert(false)
end

function screen._clear()
    assert(#screen == screen._count)
    for i=1, #screen do
        screen._deleted[screen[i]] = true
        screen[i].valid = false
        screen[screen[i]] = nil
        screen[i] = nil
    end
    screens = {}

    if mouse then
        mouse.screen = nil
    end

    screen._count = 0
end

function screen._setup_grid(w, h, rows, args)
    args = args or {}
    screen._clear()
    for i, row in ipairs(rows) do
        for j=1, row do
            args.x      = (j-1)*w + (j-1)*screen._grid_horizontal_margin
            args.y      = (i-1)*h + (i-1)*screen._grid_vertical_margin
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

    if i + 1 < #screen then
        return screen[i+1], i+1
    end
end

screen._add_screen {width=320, height=240}

screen._grid_vertical_margin = 10
screen._grid_horizontal_margin = 10

screen.primary = screen[1]

function screen.count()
    return screen._count
end

return setmetatable(screen, {
    __call  = iter_scr,
    __index = catch_invalid
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
