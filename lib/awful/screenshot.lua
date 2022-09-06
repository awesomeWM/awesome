---------------------------------------------------------------------------
--- Screenshots and related configuration settings
--
-- @author Brian Sobulefsky &lt;brian.sobulefsky@protonmail.com&gt;
-- @copyright 2021 Brian Sobulefsky
-- @classmod awful.screenshot
---------------------------------------------------------------------------

-- Grab environment we need
local capi = {
    root         = root,
    screen       = screen,
    client       = client,
    mousegrabber = mousegrabber
}

local gears     = require("gears")
local beautiful = require("beautiful")
local wibox     = require("wibox")
local cairo     = require("lgi").cairo
local abutton   = require("awful.button")
local akey      = require("awful.key")
local akgrabber = require("awful.keygrabber")
local gtimer    = require("gears.timer")
local glib      = require("lgi").GLib
local datetime  = glib.DateTime
local timezone  = glib.TimeZone

-- The module to be returned
local module = { mt = {}, _screenshot_methods = {} }
local screenshot_validation = {}

local datetime_obj = datetime.new_now

-- When $SOURCE_DATE_EPOCH and $SOURCE_DIRECTORY are both set, then this code is
-- most likely being run by the test runner. Ensure reproducible dates.
local source_date_epoch = tonumber(os.getenv("SOURCE_DATE_EPOCH"))
if source_date_epoch and os.getenv("SOURCE_DIRECTORY") then
    datetime_obj = function()
        return datetime.new_from_unix_utc(source_date_epoch)
    end
end

-- Generate a date string with the same format as the `textclock` and also with
-- support Debian reproducible builds.
local function get_date(format)
    return datetime_obj(timezone.new_local()):format(format)
end

-- Convert to a real image surface so it can be added to an imagebox.
local function to_surface(raw_surface, width, height)
    local img = cairo.ImageSurface(cairo.Format.RGB24, width, height)
    local cr = cairo.Context(img)
    cr:set_source_surface(gears.surface(raw_surface))
    cr:paint()

    return img
end

--- The screenshot interactive frame color.
-- @beautiful beautiful.screenshot_frame_color
-- @tparam[opt="#ff0000"] color screenshot_frame_color

--- The screenshot interactive frame shape.
-- @beautiful beautiful.screenshot_frame_shape
-- @tparam[opt=gears.shape.rectangle] shape screenshot_frame_shape

function screenshot_validation.directory(directory)
    -- Fully qualify a "~/" path or a relative path to $HOME. One might argue
    -- that the relative path should fully qualify to $PWD, but for a window
    -- manager this should be the same thing to the extent that anything else
    -- is arguably unexpected behavior.
    if string.find(directory, "^~/") then
        directory = string.gsub(directory, "^~/",
                              string.gsub(os.getenv("HOME"), "/*$", "/", 1))
    elseif string.find(directory, "^[^/]") then
        directory = string.gsub(os.getenv("HOME"), "/*$", "/", 1) .. directory
    end

    -- Assure that we return exactly one trailing slash
    directory = string.gsub(directory, '/*$', '/', 1)

    -- Add a trailing "/" if none is present.
    if directory:sub(-1) ~= "/" then
        directory = directory .. "/"
    end

    -- If the directory eixsts, but cannot be used, print and error.
    if gears.filesystem.is_dir(directory) and not gears.filesystem.dir_writable(directory) then
        gears.debug.print_error("`"..directory.. "` is not writable.")
    end

    return directory
end

-- Internal function to sanitize a prefix string
--
-- Currently only strips all path separators ('/'). Allows for empty prefix.
function screenshot_validation.prefix(prefix)
    if prefix:match("[/.]") then
        gears.debug.print_error("`"..prefix..
            "` is not a valid prefix because it contains `/` or `.`")
    end

    return prefix
end

-- Internal routine to verify that a file_path is valid.
function screenshot_validation.file_path(file_path)
    if gears.filesystem.file_readable(file_path) then
        gears.debug.print_error("`"..file_path.."` already exist.")
    end

    return file_path
end

-- Warn about invalid geometries.
function screenshot_validation.geometry(geo)
    for _, part in ipairs {"x", "y", "width", "height" } do
        if not geo[part] then
            gears.debug.print_error("The screenshot geometry must be a table with "..
                "`x`, `y`, `width` and `height`"
            )
            break
        end
    end

    return geo
end

function screenshot_validation.screen(scr)
    return capi.screen[scr]
end

local function make_file_name(self, method)
    local date_time = get_date(self.date_format)
    method = method and method.."_" or ""
    return self.prefix .. "_" .. method .. date_time .. ".png"
end

local function make_file_path(self, method)
    return self.directory .. (self._private.file_name or make_file_name(self, method))
end

-- Internal function to do the actual work of taking a cropped screenshot
--
-- This function does not do any data checking. Public functions should do
-- all data checking. This function was made to combine the interactive sipper
-- run by the mousegrabber and the programmatically defined snip function,
-- though there may be other uses.
local function crop_shot(source, geo)
    local target = source:create_similar(cairo.Content.COLOR, geo.width, geo.height)

    local cr = cairo.Context(target)
    cr:set_source_surface(source, -geo.x, -geo.y)
    cr:rectangle(0, 0, geo.width, geo.height)
    cr:fill()

    return target
end

-- Internal function used by the interactive mode mousegrabber to update the
-- frame outline of the current state of the cropped screenshot
--
-- This function is largely a copy of awful.mouse.snap.show_placeholder(geo),
-- so it is probably a good idea to create a utility routine to handle both use
-- cases. It did not seem appropriate to make that change as a part of the pull
-- request that was creating the screenshot API, as it would clutter and
-- confuse the change.
local function show_frame(self, surface, geo)
    local col = self._private.frame_color
          or beautiful.screenshot_frame_color
          or "#ff0000"

    local shape = self.frame_shape
        or beautiful.screenshot_frame_shape
        or gears.shape.rectangle

    local w, h = root.size()

    self._private.selection_widget = wibox.widget {
        border_width  = 3,
        border_color  = col,
        shape         = shape,
        color         = "transparent",
        visible       = false,
        widget        = wibox.widget.separator
    }
    self._private.selection_widget.point = {x=0, y=0}
    self._private.selection_widget.fit = function() return 0,0 end

    self._private.canvas_widget = wibox.widget {
        widget = wibox.layout.manual
    }

    self._private.imagebox = wibox.widget {
        image  = surface,
        widget = wibox.widget.imagebox
    }

    self._private.imagebox.point = geo
    self._private.canvas_widget:add(self._private.imagebox)
    self._private.canvas_widget:add(self._private.selection_widget)

    self._private.frame = wibox {
        ontop   = true,
        x       = 0,
        y       = 0,
        width   = w,
        height  = h,
        widget  = self._private.canvas_widget,
        visible = true,
    }
end

--- Emitted when the interactive snipping starts.
-- @signal snipping::start
-- @tparam awful.screenshot self

--- Emitted when the interactive snipping succeed.
-- @signal snipping::success
-- @tparam awful.screenshot self

--- Emitted when the interactive snipping is cancelled.
-- @signal snipping::cancelled
-- @tparam awful.screenshot self
-- @tparam string reason Either `"mouse_button"`, `"key"`, `"no_selection"`
--  or `"too_small"`.

--- Emitted when the `auto_save_delay` timer starts.
-- @signal timer::started
-- @tparam awful.screenshot self

--- Emitted after each elapsed second when `auto_save_delay` is set.
-- @signal timer::tick
-- @tparam awful.screenshot self
-- @tparam integer remaining Number of seconds remaining.

--- Emitted when the `auto_save_delay` timer stops.
--
-- This is before the screenshot is taken. If you need to hide notifications
-- or other element, it has to be done using this signal.
--
-- @signal timer::timeout
-- @tparam awful.screenshot self

-- Internal function that generates the callback to be passed to the
-- mousegrabber that implements the interactive mode.
--
-- The interactive tool is basically a mousegrabber, which takes a single function
-- of one argument, representing the mouse state data.
local function start_snipping(self, surface, geometry, method)
    self._private.mg_first_pnt = {}

    local accept_buttons, reject_buttons = {}, {}

    --TODO support modifiers.
    for _, btn in ipairs(self.accept_buttons) do
        accept_buttons[btn.button] = true
    end
    for _, btn in ipairs(self.reject_buttons) do
        reject_buttons[btn.button] = true
    end

    local pressed = false

    show_frame(self, surface, geometry)

    local function ret_mg_callback(mouse_data, accept, reject)
        for btn, status in pairs(mouse_data.buttons) do
            accept = accept or (status and accept_buttons[btn])
            reject = reject or (status and reject_buttons[btn])
        end

        if reject then
            self:reject("mouse_button")
            return false
        elseif pressed then
            local min_x = math.min(self._private.mg_first_pnt[1], mouse_data.x)
            local max_x = math.max(self._private.mg_first_pnt[1], mouse_data.x)
            local min_y = math.min(self._private.mg_first_pnt[2], mouse_data.y)
            local max_y = math.max(self._private.mg_first_pnt[2], mouse_data.y)

            self._private.selected_geometry = {
                x       = min_x,
                y       = min_y,
                width   = max_x - min_x,
                height  = max_y - min_y,
                method  = method,
                surface = surface,
            }
            self:emit_signal("property::selected_geometry", self._private.selected_geometry)

            if not accept then
                -- Released
                return self:accept()
            else
                -- Update position
                self._private.selection_widget.point.x = min_x
                self._private.selection_widget.point.y = min_y
                self._private.selection_widget.fit = function()
                    return self._private.selected_geometry.width, self._private.selected_geometry.height
                end
                self._private.selection_widget:emit_signal("widget::layout_changed")
                self._private.canvas_widget:emit_signal("widget::redraw_needed")
            end
        elseif accept then
            pressed = true
            self._private.selection_widget.visible = true
            self._private.selection_widget.point.x = mouse_data.x
            self._private.selection_widget.point.y = mouse_data.y
            self._private.mg_first_pnt[1] = mouse_data.x
            self._private.mg_first_pnt[2] = mouse_data.y
        end

        return true
    end

    self.keygrabber:start()
    capi.mousegrabber.run(ret_mg_callback, self.cursor)
    self:emit_signal("snipping::start")
end

-- Internal function exected when a root window screenshot is taken.
function module._screenshot_methods.root()
    local w, h = root.size()
    return to_surface(capi.root.content(), w, h),  {x = 0, y = 0, width = w, height = h}
end

-- Internal function executed when a physical screen screenshot is taken.
function module._screenshot_methods.screen(self)
    local geo = self.screen.geometry
    return to_surface(self.screen.content, geo.width, geo.height), geo
end

-- Internal function executed when a client window screenshot is taken.
function module._screenshot_methods.client(self)
    local c = self.client
    local bw = c.border_width
    local _, top_size = c:titlebar_top()
    local _, right_size = c:titlebar_right()
    local _, bottom_size = c:titlebar_bottom()
    local _, left_size = c:titlebar_left()

    local c_geo = c:geometry()

    local actual_geo = {
        x      = c_geo.x + left_size + bw,
        y      = c_geo.y + top_size + bw,
        width  = c_geo.width - right_size - left_size,
        height = c_geo.height - bottom_size - top_size,
    }

    return to_surface(c.content, actual_geo.width, actual_geo.height), actual_geo
end

-- Internal function executed when a snip screenshow (a defined geometry) is
-- taken.
function module._screenshot_methods.geometry(self)
    local root_w, root_h = root.size()

    local root_intrsct = gears.geometry.rectangle.get_intersection(self.geometry, {
        x      = 0,
        y      = 0,
        width  = root_w,
        height = root_h
    })

    return crop_shot(module._screenshot_methods.root(self), root_intrsct), root_intrsct
end

-- Various accessors for the screenshot object returned by any public
-- module method.

--- Get screenshot directory property.
--
-- @property directory
-- @tparam[opt=os.getenv("HOME")] string directory
-- @propemits true false

--- Get screenshot prefix property.
--
-- @property prefix
-- @tparam[opt="Screenshot-"] string prefix
-- @propemits true false

--- Get screenshot file path.
--
-- @property file_path
-- @tparam[opt=self.directory..self.prefix..os.date()..".png"] string file_path
-- @propemits true false
-- @see file_name

--- Get screenshot file name.
--
-- @property file_name
-- @tparam[opt=self.prefix..os.date()..".png"] string file_name
-- @propemits true false
-- @see file_path

--- The date format used in the default suffix.
-- @property date_format
-- @tparam[opt="%Y%m%d%H%M%S"] string date_format
-- @propemits true false
-- @see wibox.widget.textclock

--- The cursor used in interactive mode.
--
-- @property cursor
-- @tparam[opt="crosshair"] string cursor
-- @propemits true false

--- Use the mouse to select an area (snipping tool).
--
-- @property interactive
-- @tparam[opt=false] boolean interactive
-- @propemits true false

--- Get screenshot screen.
--
-- @property screen
-- @tparam[opt=nil] screen|nil screen
-- @propemits true false
-- @see mouse.screen
-- @see awful.screen.focused
-- @see screen.primary

--- Get screenshot client.
--
-- @property client
-- @tparam[opt=nil] client|nil client
-- @propemits true false
-- @see mouse.client
-- @see client.focus

--- Get screenshot geometry.
--
-- @property geometry
-- @tparam table geometry
-- @tparam table geometry.x
-- @tparam table geometry.y
-- @tparam table geometry.width
-- @tparam table geometry.height
-- @propemits true false

--- Get screenshot surface.
--
-- If none, or only one of: `screen`, `client` or `geometry` is provided, then
-- this screenshot object will have a single target image. While specifying
-- multiple target isn't recommended, you can use the `surfaces` properties
-- to access them.
--
-- Note that this is empty until either `save` or `refresh` is called.
--
-- @property surface
-- @tparam nil|image surface
-- @propertydefault `nil` if the screenshot hasn't been taken yet, a `gears.surface`
--  compatible image otherwise.
-- @readonly
-- @see surfaces

--- Get screenshot surfaces.
--
-- When multiple methods are enabled for the same screenshot, then it will
-- have multiple images. This property exposes each image individually.
--
-- Note that this is empty until either `save` or `refresh` is called. Also
-- note that you should make multiple `awful.screenshot` objects rather than
-- put multiple targets in the same object.
--
-- @property surfaces
-- @tparam[opt={}] table surfaces
-- @tparam[opt=nil] image surfaces.root The screenshot of all screens. This is
--  the default if none of `client`, screen` or `geometry` have been set.
-- @tparam[opt=nil] image surfaces.client The surface for the `client` property.
-- @tparam[opt=nil] image surfaces.screen The surface for the `screen` property.
-- @tparam[opt=nil] image surfaces.geometry The surface for the `geometry` property.
-- @readonly
-- @see surface

--- Set a minimum size to save a screenshot.
--
-- When the screenshot is very small (like 1x1 pixels), it is probably a mistake.
-- Rather than save useless files, set this property to auto-reject tiny images.
--
-- @property minimum_size
-- @tparam[opt={width=3, height=3}] nil|integer|table minimum_size
-- @tparam integer minimum_size.width
-- @tparam integer minimum_size.height
-- @propertytype nil No minimum size.
-- @propertytype integer Same minimum size for the height and width.
-- @propertytype table Different size for the height and width.
-- @negativeallowed false
-- @propemits true false

--- The interactive frame color.
-- @property frame_color
-- @tparam color|nil frame_color
-- @propbeautiful
-- @propemits true false

--- The interactive frame shape.
-- @property frame_shape
-- @tparam shape|nil frame_shape
-- @propbeautiful
-- @propemits true false

--- Define which mouse button exit the interactive snipping mode.
--
-- @property reject_buttons
-- @tparam[opt={awful.button({}, 3)}}] table reject_buttons
-- @tablerowtype A list of `awful.button` objects.
-- @propemits true false
-- @see accept_buttons

--- Mouse buttons used to save the screenshot when using the interactive snipping mode.
--
-- @property accept_buttons
-- @tparam[opt={awful.button({}, 1)}}] table accept_buttons
-- @tablerowtype A list of `awful.button` objects.
-- @propemits true false
-- @see reject_buttons

--- The `awful.keygrabber` object used for the accept and reject keys.
--
-- This can be used to add new keybindings to the interactive snipper mode. For
-- examples, this can be used to change the saving path or add some annotations
-- to the image.
--
-- @property keygrabber
-- @tparam awful.keygrabber keygrabber
-- @propertydefault Autogenerated.
-- @readonly

--- The current interactive snipping mode seletion.
-- @property selected_geometry
-- @tparam nil|table selected_geometry
-- @tparam integer selected_geometry.x
-- @tparam integer selected_geometry.y
-- @tparam integer selected_geometry.width
-- @tparam integer selected_geometry.height
-- @tparam image selected_geometry.surface
-- @tparam string selected_geometry.method Either `"root"`, `"client"`,
--  `"screen"` or `"geometry"`.
-- @propemits true false
-- @readonly

--- Number of seconds before auto-saving (or entering the interactive snipper).
--
-- You can use `0` to auto-save immediatly.
--
-- @property auto_save_delay
-- @tparam[opt=nil] integer|nil auto_save_delay
-- @propertytype nil Do not auto-save.
-- @propertyunit second
-- @negativeallowed false
-- @propemits true false

--- Duration between the `"timer::tick"` signals when using `auto_save_delay`.
-- @property auto_save_tick_duration
-- @tparam[opt=1.0] number auto_save_tick_duration
-- @propertyunit second
-- @negativeallowed false
-- @propemits true false

local defaults = {
    prefix                  = "Screenshot-",
    directory               = screenshot_validation.directory(os.getenv("HOME")),
    cursor                  = "crosshair",
    date_format             = "%Y%m%d%H%M%S",
    interactive             = false,
    reject_buttons          = {abutton({}, 3)},
    accept_buttons          = {abutton({}, 1)},
    reject_keys             = {akey({}, "Escape")},
    accept_keys             = {akey({}, "Return")},
    minimum_size            = {width = 3, height = 3},
    auto_save_tick_duration = 1,
}

-- Create the standard properties.
for _, prop in ipairs { "frame_color", "geometry", "screen", "client", "date_format",
                        "prefix", "directory", "file_path", "file_name", "auto_save_delay",
                        "interactive", "reject_buttons", "accept_buttons", "cursor",
                        "reject_keys", "accept_keys", "frame_shape", "minimum_size",
                        "auto_save_tick_duration" } do
    module["set_"..prop] = function(self, value)
        self._private[prop] = screenshot_validation[prop]
            and screenshot_validation[prop](value) or value
        self:emit_signal("property::"..prop, value)
    end

    module["get_"..prop] = function(self)
        return self._private[prop] or defaults[prop]
    end
end

function module:get_selected_geometry()
    return self._private.selected_geometry
end

function module:get_file_path()
    return self._private.file_path or make_file_path(self)
end

function module:get_file_name()
    return self._private.file_name or make_file_name(self)
end

function module:get_surface()
    return self.surfaces[1]
end

function module:get_surfaces()
    local ret = {}

    for method, surface in pairs(self._private.surfaces or {}) do
        ret[method] = surface.surface
    end

    return ret
end

function module:get_surfaces()
    local ret = {}

    for _, surface in pairs(self._private.surfaces or {}) do
        table.insert(ret, surface.surface)
    end

    return ret
end

function module:get_keygrabber()
    if self._private.keygrabber then return  self._private.keygrabber end

    self._private.keygrabber = akgrabber {
        stop_key = self.reject_buttons
    }
    self._private.keygrabber:connect_signal("keybinding::triggered", function(_, key, event)
        if event == "press" then return end
        if self._private.accept_keys_set[key] then
            self:accept()
        elseif self._private.reject_keys_set[key] then
            self:reject()
        end
    end)

    -- Reload the keys.
    self.accept_keys, self.reject_keys = self.accept_keys, self.reject_keys

    return self._private.keygrabber
end

-- Put the key in a set rather than a list and add/remove them from the keygrabber.
for _, prop in ipairs {"accept_keys", "reject_keys"} do
    local old = module["set_"..prop]

    module["set_"..prop] = function(self, new_keys)
        -- Remove old keys.
        if self._private.keygrabber then
            for _, key in ipairs(self._private[prop] or {}) do
                self._private.keygrabber:remove_keybinding(key)
            end
        end

        local new_set = {}
        self._private[prop.."_set"] = new_set

        for _, key in ipairs(new_keys) do
            self.keygrabber:add_keybinding(key)
            new_set[key] = true
        end

        old(self, new_keys)
    end
end

function module:set_minimum_size(size)
    if size and type(size) ~= "table" then
        size = {width = math.ceil(size), height = math.ceil(size)}
    end
    self._private.minimum_size = size
    self:emit_signal("property::minimum_size", size)
end

function module:set_auto_save_delay(value)
    self._private.auto_save_delay = math.ceil(value)
    self:emit_signal("property::auto_save_delay", value)

    if not value then
        if self._private.timer then
            self._private.timer:stop()
            self._private.timer = nil
        end
        return
    end

    if value == 0 then
        self:refresh()
        if not self.interactive then
            self:save()
        end
        return
    end

    self._private.current_delay = self._private.auto_save_delay

    if not self._private.timer then
        local dur = self.auto_save_tick_duration
        self._private.timer = gtimer {timeout = dur}
        local fct
        fct = function()
            self._private.current_delay = self._private.current_delay - dur
            self:emit_signal("timer::tick", self._private.current_delay)

            if self._private.current_delay <= 0 then
                self:emit_signal("timer::timeout")
                awesome.sync()
                self:refresh()

                if not self.interactive then
                    self:save()
                end

                -- For garbage collection of the `awful.screenshot` object.
                self._private.timer:stop()
                self._private.timer:disconnect_signal("timeout", fct)
                self._private.timer = nil
            end
        end
        self._private.timer:connect_signal("timeout", fct)
    end

    self._private.timer:again()
    self:emit_signal("timer::started")
end

--- Take new screenshot(s) now.
--
--
-- @method refresh
-- @treturn table A table with the method name as key and the images as value.
-- @see save

function module:refresh()
    local methods = {}
    self._private.surfaces = {}

    for method in pairs(module._screenshot_methods) do
        if self._private[method] then
            table.insert(methods, method)
        end
    end

    -- Fallback to a screenshot of everything.
    methods = #methods > 0 and methods or {"root"}

    for _, method in ipairs(methods) do
        local surface, geo = module._screenshot_methods[method](self)

        if self.interactive then
            start_snipping(self, surface, geo, method)
        else
            self._private.surfaces[method] = {surface = surface, geometry = geo}
        end

    end

    return self.surfaces
end

--- Emitted when a the screenshot is saved.
--
-- This can be due to `:save()` being called, the `interactive` mode finishing
-- or `auto_save_delay` timing out.
--
-- @signal file::saved
-- @tparam awful.screenshot self The screenshot.
-- @tparam string file_path The path.
-- @tparam[opt] string|nil method The method associated with the file_path. This
--  can be `root`, `geometry`, `client` or `screen`.

--- Save screenshot.
--
-- @method save
-- @tparam[opt=self.file_path] string file_path Optionally override the file path.
-- @noreturn
-- @emits saved
-- @see refresh
function module:save(file_path)
    if not self._private.surfaces then self:refresh() end

    for method, surface in pairs(self._private.surfaces) do
        file_path = file_path
            or self._private.file_path
            or make_file_path(self, #self._private.surfaces > 1 and method or nil)

        surface.surface:write_to_png(file_path)
        self:emit_signal("file::saved", file_path, method)
    end
end

--- Save and exit the interactive snipping mode.
-- @method accept
-- @treturn boolean `true` if the screenshot were save, `false` otherwise. It
--  can be `false` because the selection is below `minimum_size` or because
--  there is nothing to save (so selection).
-- @emits snipping::cancelled
-- @emits snipping::success

function module:accept()
    -- Nothing to do.
    if not self.interactive then return true end

    local new_geo = self._private.selected_geometry
    local min_size = self.minimum_size

    if not new_geo then
        self:reject("no_selection")
        return false
    end

    -- This may fail gracefully anyway but require a minimum 3x3 of pixels
    if min_size and new_geo.width < min_size.width or  new_geo.height < min_size.height then
        self:reject("too_small")
        return false
    end

    self._private.selection_widget.visible = false

    self._private.surfaces[new_geo.method] = {
        surface  = crop_shot(new_geo.surface, new_geo),
        geometry = new_geo
    }

    self:emit_signal("snipping::success")
    self:save()

    self._private.frame.visible = false
    self._private.frame, self._private.mg_first_pnt = nil, nil
    self.keygrabber:stop()
    capi.mousegrabber.stop()

    return true
end

--- Exit the interactive snipping mode without saving.
-- @method reject
-- @tparam[opt=nil] string||nil reason The reason why it was rejected. This is
--  passed to the `"snipping::cancelled"` signal.
-- @emits snipping::cancelled

function module:reject(reason)
    -- Nothing to do.
    if not self.interactive then return end

    if self._private.frame then
        self._private.frame.visible = false
        self._private.frame = nil
    end
    self._private.mg_first_pnt = nil
    self:emit_signal("snipping::cancelled", reason or "reject_called")
    capi.mousegrabber.stop()
    self.keygrabber:stop()
end

--- Screenshot constructor - it is possible to call this directly, but it is.
--  recommended to use the helper constructors, such as awful.screenshot.root
--
-- @constructorfct awful.screenshot
-- @tparam[opt={}] table args
-- @tparam[opt] string args.directory Get screenshot directory property.
-- @tparam[opt] string args.prefix Get screenshot prefix property.
-- @tparam[opt] string args.file_path Get screenshot file path.
-- @tparam[opt] string args.file_name Get screenshot file name.
-- @tparam[opt] screen args.screen Get screenshot screen.
-- @tparam[opt] string args.date_format The date format used in the default suffix.
-- @tparam[opt] string args.cursor The cursor used in interactive mode.
-- @tparam[opt] boolean args.interactive Rather than take a screenshot of an
--  object, use the mouse to select an area.
-- @tparam[opt] client args.client Get screenshot client.
-- @tparam[opt] table args.geometry Get screenshot geometry.
-- @tparam[opt] color args.frame_color The frame color.

local function new(_, args)
    args = (type(args) == "table" and args) or {}
    local self = gears.object({
        enable_auto_signals = true,
        enable_properties   = true
    })

    self._private = {}
    gears.table.crush(self, module, true)
    gears.table.crush(self, args)

    return self
end

return setmetatable(module, {__call = new})
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
