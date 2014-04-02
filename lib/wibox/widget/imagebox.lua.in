---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local surface = require("gears.surface")
local setmetatable = setmetatable
local pairs = pairs
local type = type
local pcall = pcall
local print = print

-- wibox.widget.imagebox
local imagebox = { mt = {} }

--- Draw an imagebox with the given cairo context in the given geometry.
function imagebox:draw(wibox, cr, width, height)
    if not self._image then return end
    if width == 0 or height == 0 then return end

    cr:save()

    if not self.resize_forbidden then
        -- Let's scale the image so that it fits into (width, height)
        local w = self._image:get_width()
        local h = self._image:get_height()
        local aspect = width / w
        local aspect_h = height / h
        if aspect > aspect_h then aspect = aspect_h end

        cr:scale(aspect, aspect)
    end
    cr:set_source_surface(self._image, 0, 0)
    cr:paint()

    cr:restore()
end

--- Fit the imagebox into the given geometry
function imagebox:fit(width, height)
    if not self._image then
        return 0, 0
    end

    local w = self._image:get_width()
    local h = self._image:get_height()

    if w > width then
        h = h * width / w
        w = width
    end
    if h > height then
        w = w * height / h
        h = height
    end

    if h == 0 or w == 0 then
        return 0, 0
    end

    if not self.resize_forbidden then
        local aspect = width / w
        local aspect_h = height / h

        -- Use the smaller one of the two aspect ratios.
        if aspect > aspect_h then aspect = aspect_h end

        w, h = w * aspect, h * aspect
    end

    return w, h
end

--- Set an imagebox' image
-- @param image Either a string or a cairo image surface. A string is
--              interpreted as the path to a png image file.
function imagebox:set_image(image)
    local image = image

    if type(image) == "string" then
        local success, result = pcall(surface.load, image)
        if not success then
            print("Error while reading '" .. image .. "': " .. result)
            return false
        end
        image = result
    end

    image = surface.load(image)

    if image then
        local w = image.width
        local h = image.height
        if w <= 0 or h <= 0 then
            return false
        end
    end

    self._image = image

    self:emit_signal("widget::updated")
    return true
end

--- Should the image be resized to fit into the available space?
-- @param allowed If false, the image will be clipped, else it will be resized
--                to fit into the available space.
function imagebox:set_resize(allowed)
    self.resize_forbidden = not allowed
    self:emit_signal("widget::updated")
end

-- Returns a new imagebox
-- @param image the image to display, may be nil
-- @param resize_allowed If false, the image will be clipped, else it will be resized
--                       to fit into the available space.
local function new(image, resize_allowed)
    local ret = base.make_widget()

    for k, v in pairs(imagebox) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    if image then
        ret:set_image(image)
    end
    if resize_allowed ~= nil then
        ret:set_resize(resize_allowed)
    end

    return ret
end

function imagebox.mt:__call(...)
    return new(...)
end

return setmetatable(imagebox, imagebox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
