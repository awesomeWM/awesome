local gears_obj = require("gears.object")
local gears_tab = require("gears.table")

local screen, meta = awesome._shim_fake_class()
screen._count, screen._deleted = 0, {}

local function compute_workarea(s)
    local struts = {top=0,bottom=0,left=0,right=0}

    for _, c in ipairs(drawin.get()) do
        for k,v in pairs(struts) do
            struts[k] = v + (c:struts()[k] or 0)
        end
    end

    for _, c in ipairs(client.get()) do
        for k,v in pairs(struts) do
            struts[k] = v + (c:struts()[k] or 0)
        end
    end

    return {
        x      = s.geometry.x      + struts.left,
        y      = s.geometry.y      + struts.top ,
        width  = s.geometry.width  - struts.left - struts.right ,
        height = s.geometry.height - struts.top  - struts.bottom,
    }
end

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

    function s.fake_resize(self, x, y, width, height)
        self._resize {
            x=x,y=y,width=width,height=height
        }
    end

    function s:fake_remove()
        local i = s.index
        table.remove(screen, i)
        screen._deleted[s] = true
        s:emit_signal("removed")
        screen[screen[i]] = nil
        s.valid = false
    end

    function s:swap(other_s)
        local s1geo = gears_tab.clone(s.geometry)
        local s2geo = gears_tab.clone(other_s.geometry)

        s:fake_resize(
            s2geo.x, s2geo.y, s2geo.width, s2geo.height
        )
        other_s:fake_resize(
            s1geo.x, s1geo.y, s1geo.width, s1geo.height
        )

        s:emit_signal("swapped",other_s)
        other_s:emit_signal("swapped",s)
    end

    s.outputs = { LVDS1 ={
        name      = "LVDS1",
        mm_width  = (geo.width /96)*25.4,
        mm_height = (geo.height/96)*25.4,
    }}

    local wa = args.workarea_sides or 10

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
                if screen._track_workarea then
                    return compute_workarea(s)
                else
                    return {
                        x      = (geo.x or 0) + wa  ,
                        y      = (geo.y or 0) + wa  ,
                        width  = geo.width    - 2*wa,
                        height = geo.height   - 2*wa,
                    }
                end
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

    return s
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
    if s == nil then return end

    -- Try to get the screens by output name.
    if type(s) == "string" then
        for s2 in screen do
            for out in pairs(s2.outputs or {}) do
                if out == s then
                    return s2
                end
            end
        end
    end

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

    if i < #screen then
        return screen[i+1], i+1
    end
end

function screen._viewports()
    return {}
end

function screen.fake_add(x,y,width,height)
    return screen._add_screen {
        x=x,y=y,width=width,height=height
    }
end

screen._add_screen {width=320, height=240}

screen._grid_vertical_margin = 10
screen._grid_horizontal_margin = 10

screen.primary = screen[1]

screen._track_workarea = false

function screen.count()
    return screen._count
end

return setmetatable(screen, {
    __call  = iter_scr,
    __index = catch_invalid
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
