-------------------------------------------------------------------------
-- @author Sébastien Gross &lt;seb•ɱɩɲʋʃ•awesome•ɑƬ•chezwam•ɖɵʈ•org&gt
-- @copyright 2009 Sébastien Gross
-- @release @AWESOME_VERSION@
-------------------------------------------------------------------------

local mouse = mouse
local screen = screen
local timer = timer
local wibox = require("wibox")
local a_placement = require("awful.placement")
local a_wibox = require("awful.wibox")
local beautiful = require("beautiful")
local setmetatable = setmetatable

--- Find the mouse pointer on the screen.
-- Mouse finder highlights the mouse cursor on the screen
-- <p>To enable this feature, a <code>awful.mouse.finder</code> object needs to
-- be bound to a key:<br/>
-- <code>mymousefinder = awful.mouse.finder()</code><br/>
-- Then bind the <code>find</code> function a key binding.
-- <p>Some configuration variable can be set in the theme:<br/>
-- The mouse_finder display duration<br/>
-- <code>theme.mouse_finder_timeout = 3</code><br/>
-- The animation speed<br/>
-- <code>theme.mouse_finder_animate_timeout = 0.05</code><br/>
-- The mouse_finder radius<br/>
-- <code>theme.mouse_finder_radius = 20</code><br/>
-- The growth factor<br/>
-- <code>theme.mouse_finder_factor = 2</code><br/>
-- The mouse_finder color<br/>
-- <code>theme.mouse_finder_color = "#ff0000"</code><br/>
-- </p>

-- awful.mouse.finder
local finder = { mt = {} }

-- Mouse finder private data.
-- @name data
-- @field color Background color.
-- @field hide The hide() function.
-- @field show The show() function.
-- @field timer Timer to hide the mouse finder.
-- @field animate_timer Timer to animate the mouse finder.
-- @field wibox The mouse finder wibox show on the screen.
local data = setmetatable({}, { __mode = 'k' })

-- Place a mouse finder on the screen.
-- @param self A mouse finder object.
local function place(self)
    a_placement.under_mouse(data[self].wibox)
    a_placement.no_offscreen(data[self].wibox)
end

-- Animate a mouse finder.
-- @param self A mouse finder object.
local function animate(self)
    local r = data[self].wibox:geometry().width
    -- Check if the object should be grown or shrinked
    -- the minimum radius is -data[self].factor because:
    -- 1. factor is alway negative when shrinking
    -- 2. geometry() does not hande negative values
    if data[self].factor > 0 and r >= data[self].radius
        or data[self].factor < 0 and r <= -data[self].factor then
            data[self].factor = -data[self].factor
    end
    data[self].wibox:geometry({width = r + data[self].factor,
        height = r + data[self].factor })
    -- need -1 to the radius to draw a full circle
    -- FIXME: The rounded_corners() API was removed
    -- a_wibox.rounded_corners(data[self].wibox, (r + data[self].factor)/2 -1)
    -- make sure the mouse finder follows the pointer. Uh!
    place(self)
end


-- Show a mouse finder.
-- @param self The mouse finder to show.
local function show(self)
    -- do nothing if the mouse finder is already shown
    if data[self].wibox.visible then return end
    if not data[self].timer.started then
        data[self].wibox:geometry({width = data[self].radius, height = data[self].radius })
        -- FIXME: The rounded_corners() API was removed
        -- a_wibox.rounded_corners(data[self].wibox, data[self].radius/2 -1)
        data[self].timer:start()
        data[self].animate_timer:start()
    end
    place(self)
    data[self].wibox.visible = true
end

-- Hide a mouse finder.
-- @param self The mouse finder to hide.
local function hide(self)
    -- do nothing if the mouse finder is already hidden
    if not data[self].wibox.visible then return end
    if data[self].timer.started then
            data[self].timer:stop()
            data[self].animate_timer:stop()
    end
    data[self].wibox.visible = false
end

-- Load Default values.
-- @param self A mouse finder object.
local function set_defaults(self)
    data[self].wibox.border_width = 0
    data[self].wibox.opacity = beautiful.mouse_finder_opacity or 1
    data[self].wibox.bg = beautiful.mouse_finder_color or beautiful.bg_focus or "#ff0000"
    data[self].timeout = beautiful.mouse_finder_timeout or 3
    data[self].animate_timeout = beautiful.mouse_finder_animate_timeout or 0.05
    data[self].radius = beautiful.mouse_finder_radius or 20
    data[self].factor = beautiful.mouse_finder_factor or 2
end

--- Find the mouse on the screen
-- @param self A mouse finder object.
function finder.find(self)
    show(self)
end

--- Create a new mouse finder.
local function new()
    local self = { }
    -- private data
    data[self] = {
        wibox =  wibox({ }),
        show = function() show(self) end,
        hide = function() hide(self) end,
        animate = function() animate(self) end,
    }

    -- export functions
    self.find = find

    set_defaults(self)

    -- setup the timer action only if needed
    data[self].timer = timer { timeout = data[self].timeout }
    data[self].animate_timer = timer { timeout = data[self].animate_timeout }
    data[self].timer:connect_signal("timeout", data[self].hide)
    data[self].animate_timer:connect_signal("timeout", data[self].animate)
    data[self].wibox.ontop = true
    data[self].wibox.visible = false

    return self
end

function finder.mt:__call(...)
    return new(...)
end

return setmetatable(finder, finder.mt)

-- vim: ft=lua:et:sw=4:ts=4:sts=4:enc=utf-8:tw=78
