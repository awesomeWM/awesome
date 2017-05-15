-- FIXME See commit 7901a1c6479aea
local skip_wallpapers_due_to_weird_slowdowns = true

-- Helper module to replicate a set of steps across multiple screen scenarios
-- Ideas for improvements:
--  * Export an iterator so the suits can call the screen change step by step
--    themselves instead of the current all or nothing
--  * Offer multiple scenarios such as screen add, remove, resize/swap instead
--    of having to test all corner cases all the time.
local wibox = require("wibox")
local surface = require("gears.surface")
local wallpaper = require("gears.wallpaper")
local color = require("gears.color")
local shape = require("gears.shape")

local module = {}

-- Get the original canvas size before it is "lost"
local canvas_w, canvas_h = root.size()

local half_w, half_h = math.floor(canvas_w/2), math.floor(canvas_h/2)
local quarter_w = math.floor(canvas_w/4)

-- Create virtual screens.
-- The steps will be executed on each set of screens.
local dispositions = {
    -- A simple classic, 2 identical screens side by side
    {
        function()
            return {
                x = 0,
                y = 0,
                width  = half_w,
                height = canvas_h,
            }
        end,
        function()
            return {
                x = half_w,
                y = 0,
                width = half_w,
                height = canvas_h,
            }
        end,
    },

    -- Take the previous disposition and swap the screens
    {
        function()
            return {
                x = half_w,
                y = 0,
                width = half_w,
                height = canvas_h,
            }
        end,
        function()
            return {
                x = 0,
                y = 0,
                width  = half_w,
                height = canvas_h,
            }
        end,
        keep = true,
    },

    -- Take the previous disposition and resize the leftmost one
    {
        function()
            return {
                x = quarter_w,
                y = 0,
                width = 3*quarter_w,
                height = canvas_h,
            }
        end,
        function()
            return {
                x = 0,
                y = 0,
                width  = quarter_w,
                height =  half_h,
            }
        end,
        keep = true,
    },

    -- Take the previous disposition and remove the leftmost screen
    {
        function()
            return {
                x = quarter_w,
                y = 0,
                width = 3*quarter_w,
                height = canvas_h,
            }
        end,
        function()
            return nil
        end,
        keep = true,
    },

    -- Less used, but still quite common vertical setup
    {
        function()
            return {
                x = 0,
                y = 0,
                width  = canvas_w,
                height = half_h,
            }
        end,
        function()
            return {
                x = 0,
                y = half_h,
                width  = canvas_w,
                height = half_h,
            }
        end
    },

    -- Another popular setup, 22" screens with a better 31" in the middle.
    -- So, 2 smaller vertical screen with a larger central horizontal one.
    {
        -- Left
        function()
            return {
                x = 0,
                y = 0,
                width  = quarter_w,
                height = canvas_h,
            }
        end,
        -- Right, this test non-continous screen index on purpose, as this setup
        -- Often requires dual-GPU, it _will_ happen.
        function()
            return {
                x = canvas_w-quarter_w,
                y = 0,
                width = quarter_w,
                height = canvas_h,
            }
        end,
        -- Center
        function()
            return {
                x = quarter_w,
                y = 0,
                width  = half_w,
                height = math.floor(canvas_h*(9/16)),
            }
        end,
    },

    -- Same as the previous one, but with the gap centered
    {
        -- Left
        function()
            return {
                x = 0,
                y = 0,
                width  = quarter_w,
                height = canvas_h,
            }
        end,
        -- Right, this test non-continous screen index on purpose, as this setup
        -- Often requires dual-GPU, it _will_ happen.
        function()
            return {
                x = canvas_w-quarter_w,
                y = 0,
                width = quarter_w,
                height = canvas_h,
            }
        end,
        -- Center
        function()
            return {
                x = quarter_w,
                y = math.ceil((canvas_h-(canvas_h*(9/16)))/2),
                width  = half_w,
                height = math.floor(canvas_h*(9/16)),
            }
        end,

        -- Keep the same screens as the last test, just move them
        keep = true,
    },

    -- AMD Eyefinity / (old) NVIDIA MOSAIC style config (symmetric grid)
    {
        function()
            return {
                x = 0,
                y = 0,
                width  = half_w,
                height = half_h,
            }
        end,
        function()
            return {
                x = half_w,
                y = 0,
                width = half_w,
                height = half_h,
            }
        end,
        function()
            return {
                x = 0,
                y = half_h,
                width  = half_w,
                height = half_h,
            }
        end,
        function()
            return {
                x = half_w,
                y = half_h,
                width = half_w,
                height = half_h,
            }
        end,
    },

    -- Corner case 1: Non-continuous screens
    -- If there is nothing and client is drag&dropped into that area, some
    -- geometry callbacks may break in nil index.
    {
        function()
            return {
                x = 0,
                y = 0,
                width  = quarter_w,
                height = canvas_h,
            }
        end,
        function()
            return {
                x = canvas_w-quarter_w,
                y = 0,
                width = quarter_w,
                height = canvas_h,
            }
        end,
    },

    -- Corner case 2a: Nothing at 0x0.
    -- As some position may fallback to 0x0 this need to be tested often. It
    -- also caused issues such as #154
    {
        function()
            return {
                x = 0,
                y = half_w,
                width  = half_w,
                height = half_w,
            }
        end,
        function()
            return {
                x = half_w,
                y = 0,
                width = half_w,
                height = canvas_h,
            }
        end
    },

    -- Corner case 2b: Still nothing at 0x0
    {
        function() return { x = 0, y = 32, width  = 32, height = 32, } end,
        function() return { x = 32, y = 0, width = 32, height = 32, } end,
        function() return { x = 64, y = 16, width = 32, height = 32, } end,
    },

    -- Corner case 3: Many very small screens.
    -- On the embedded side of the compuverse, it is possible
    -- to buy 32x32 RGB OLED screens. They are usually used to display single
    -- status icons, but why not use them as a desktop! This is a critical
    -- market for AwesomeWM. Here's a 256px wide strip of tiny screens.
    -- This may cause wibars to move to the wrong screen accidentally
    {
        function() return { x = 0  , y = 0, width  = 32, height = 32, } end,
        function() return { x = 32 , y = 0, width  = 32, height = 32, } end,
        function() return { x = 64 , y = 0, width  = 32, height = 32, } end,
        function() return { x = 96 , y = 0, width  = 32, height = 32, } end,
        function() return { x = 128, y = 0, width  = 32, height = 32, } end,
        function() return { x = 160, y = 0, width  = 32, height = 32, } end,
        function() return { x = 192, y = 0, width  = 32, height = 32, } end,
        function() return { x = 224, y = 0, width  = 32, height = 32, } end,
    },

    -- Corner case 4: A screen taller than more than 1 other screen.
    -- this may cause some issues with client resize and drag&drop move
    {
        function()
            return {
                x = 0,
                y = 0,
                width  = half_w,
                height = canvas_h,
            }
        end,
        function()
            return {
                x = half_w,
                y = 0,
                width = half_w,
                height = math.floor(canvas_h/3),
            }
        end,
        function()
            return {
                x = half_w,
                y = math.floor(canvas_h/3),
                width = half_w,
                height = math.floor(canvas_h/3),
            }
        end,
        function()
            return {
                x = half_w,
                y = 2*math.floor(canvas_h/3),
                width = half_w,
                height = math.floor(canvas_h/3),
            }
        end
    },

    -- The overlapping corner case isn't supported. There is valid use case,
    -- such as a projector with a smaller resolution than the laptop screen
    -- in non-scaling mirror mode, but it isn't worth the complexity it brings.
}

local function check_tag_indexes()
    for s in screen do
        for i, t in ipairs(s.tags) do
            assert(t.index  == i)
            assert(t.screen == s)
        end
    end
end

local colors = {
    "#000030",
    "#300000",
    "#043000",
    "#302E00",
    "#002C30",
    "#300030",
    "#301C00",
    "#140030",
}

-- Paint it black
local function clear_screen()
    if skip_wallpapers_due_to_weird_slowdowns then
        return
    end
    for s in screen do
        local sur = surface.widget_to_surface(
            wibox.widget {
                bg     = "#000000",
                widget = wibox.container.background
            },
            s.geometry.width,
            s.geometry.height
        )
        wallpaper.fit(sur, s, "#000000")
    end
end

-- Make it easier to debug the tests by showing the screen geometry when the
-- tests are executed.
local function show_screens()
    if skip_wallpapers_due_to_weird_slowdowns then
        wallpaper.maximized = function() end
        return
    end
    wallpaper.set(color("#000000")) -- Should this clear the wallpaper? It doesn't

    -- Add a wallpaper on each screen
    for i=1, screen.count() do
        local s = screen[i]

        local w = wibox.widget {
            {
                text   = table.concat{
                    "Screen: ",i,"\n",
                    s.geometry.width,"x",s.geometry.height,
                    "+",s.geometry.x,",",s.geometry.y
                },
                valign = "center",
                align  = "center",
                widget = wibox.widget.textbox,
            },
            bg                 = colors[i],
            fg                 = "#ffffff",
            shape_border_color = "#ff0000",
            shape_border_width = 1,
            shape              = shape.rectangle,
            widget             = wibox.container.background
        }
        local sur = surface.widget_to_surface(
            w,
            s.geometry.width,
            s.geometry.height
        )
        wallpaper.fit(sur, s)
    end
end

local function add_steps(real_steps, new_steps)
    for _, dispo in ipairs(dispositions) do
        -- Cleanup
        table.insert(real_steps, function()
            if #client.get() == 0 then return true end

            for _, c in ipairs(client.get()) do
                c:kill()
            end
        end)

        table.insert(real_steps, function()
        clear_screen()
        local keep = dispo.keep or false
        local old = {}
        local geos = {}

        check_tag_indexes()

        if keep then
            for _, sf in ipairs(dispo) do
                local geo = sf and sf() or nil

                -- If the function return nothing, assume the screen need to
                -- be destroyed.
                table.insert(geos, geo or false)
            end

            -- Removed screens need to be explicit.
            assert(#geos >= screen.count())

            -- Keep a cache to avoid working with invalid data
            local old_screens = {}
            for s in screen do
                table.insert(old_screens, s)
            end

            for i, s in ipairs(old_screens) do
                -- Remove the screen (if no new geometry is given
                if not geos[i] then
                    s:fake_remove()
                else
                    local cur_geo = s.geometry
                    for _, v in ipairs {"x", "y", "width", "height" } do
                        cur_geo[v] = geos[i][v] or cur_geo[v]
                    end
                    s:fake_resize(
                        cur_geo.x,
                        cur_geo.y,
                        cur_geo.width,
                        cur_geo.height
                    )
                end
            end

            -- Add the remaining screens
            for i=#old_screens + 1, #geos do
                local geo = geos[i]
                screen.fake_add(geo.x, geo.y, geo.width, geo.height)
            end

            check_tag_indexes()
        else
            -- Move all existing screens out of the way (to avoid temporary overlapping)
            for s in screen do
                s:fake_resize(canvas_w, canvas_h, 1, 1)
                table.insert(old, s)
            end

            -- Add the new screens
            for _, sf in ipairs(dispo) do
                local geo = sf()
                screen.fake_add(geo.x, geo.y, geo.width, geo.height)
                table.insert(geos, geo)
            end

            -- Remove old screens
            for _, s in ipairs(old) do
                s:fake_remove()
            end
        end

        show_screens()

        -- Check the result is correct
        local expected_count = 0
        for _,v in ipairs(geos) do
            expected_count = expected_count + (v and 1 or 0)
        end

        assert(expected_count == screen.count())
        for k, geo in ipairs(geos) do
            if geo then
                local sgeo = screen[k].geometry
                assert(geo.x == sgeo.x)
                assert(geo.y == sgeo.y)
                assert(geo.width  == sgeo.width )
                assert(geo.height == sgeo.height)
            end
        end

        return true
    end)

        for _, step in ipairs(new_steps) do
            table.insert(real_steps, step)
        end
    end
end

-- This is a very ugly hack to speed up the test. Luacov get exponentially
-- slower / more memory hungry when it has more work to do in a single test.
-- A lot of that work is building wibars and widgets for those screen. This
-- has value when tried once (already covered by the test-screen-changes suit),
-- but not done 180 times in a row. This code monkey-patch `wibox` with the
-- intent of breaking it without causing errors. This way it stops doing too
-- many things (resulting in a faster luacov execution)
function module.disable_wibox()
    local awful = require("awful")

    setmetatable(wibox, {
        __call = function() return {
            geometry = function()
                return{x=0, y=0, width=0, height=0}
            end,
            set_widget = function() end,
            setup = function() return {} end
        }
    end })

    awful.wibar = wibox
end

return setmetatable(module, {
                    __call = function(_,...) return add_steps(...) end
                })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
