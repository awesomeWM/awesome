---------------------------------------------------------------------------
-- A StatusNotifierItem (a tray icon exported over D-Bus by an application).
--
-- Objects of this class are created by `wibox.widget.systray.statusnotifier` and should not
-- be created manually.
--
-- @author WooParadog
-- @copyright 2026 WooParadog
-- @classmod wibox.widget.systray.item
-- @supermodule gears.object
---------------------------------------------------------------------------

local lgi = require("lgi")
local Gio, GLib = lgi.Gio, lgi.GLib
local gobject = require("gears.object")
local gtable = require("gears.table")
local gtimer = require("gears.timer")
local gdebug = require("gears.debug")
local protected_call = require("gears.protected_call")
local icon = require("wibox.widget.systray.icon")

local capi = { mouse = mouse }

local item = {}

--- The application provided identifier (usually the application name).
-- @property id
-- @tparam[opt=""] string id
-- @propemits true false
-- @readonly

--- The item category.
--
-- One of `ApplicationStatus`, `Communications`, `SystemServices` and
-- `Hardware`.
--
-- @property category
-- @tparam[opt="ApplicationStatus"] string category
-- @propemits true false
-- @readonly

--- The item status.
--
-- One of `Passive` (not important), `Active` and `NeedsAttention`.
--
-- @property status
-- @tparam[opt="Active"] string status
-- @propemits true false
-- @readonly

--- The human readable item title.
-- @property title
-- @tparam[opt=""] string title
-- @propemits true false
-- @readonly

--- The X11 window id of the application window, if any.
-- @property window_id
-- @tparam[opt=0] integer window_id
-- @negativeallowed false
-- @propemits true false
-- @readonly

--- The freedesktop icon name.
-- @property icon_name
-- @tparam[opt=""] string icon_name
-- @propemits true false
-- @readonly

--- An additional application specific icon theme path.
-- @property icon_theme_path
-- @tparam[opt=""] string icon_theme_path
-- @propemits true false
-- @readonly

--- The icon name used when the status is `NeedsAttention`.
-- @property attention_icon_name
-- @tparam[opt=""] string attention_icon_name
-- @propemits true false
-- @readonly

--- The tooltip.
--
-- A table with the `icon_name`, `title` and `text` keys. The text may
-- contain markup.
--
-- @property tooltip
-- @tparam[opt=nil] table|nil tooltip
-- @propemits true false
-- @readonly

--- True if the item only supports showing a context menu.
-- @property item_is_menu
-- @tparam[opt=false] boolean item_is_menu
-- @propemits true false
-- @readonly

--- The object path of the `com.canonical.dbusmenu` menu, if any.
-- @property menu_path
-- @tparam[opt=nil] string|nil menu_path
-- @propemits true false
-- @readonly

--- The D-Bus service (bus name) of the item.
-- @property service
-- @tparam[opt=""] string service
-- @readonly

--- The D-Bus object path of the item.
-- @property path
-- @tparam[opt="/StatusNotifierItem"] string path
-- @readonly

--- Emitted after the properties have been (re)loaded and something changed.
-- @signal updated

--- Emitted when the item goes away.
-- @signal removed

--- Emitted once the properties have been loaded for the first time.
-- @signal ready
-- @hidden

-- Map D-Bus property names to Lua property names.
local property_names = {
    Id                  = "id",
    Category            = "category",
    Status              = "status",
    Title               = "title",
    WindowId            = "window_id",
    IconName            = "icon_name",
    IconThemePath       = "icon_theme_path",
    AttentionIconName   = "attention_icon_name",
    ItemIsMenu          = "item_is_menu",
    Menu                = "menu_path",
}

-- Properties which carry binary data and must be kept as raw variants.
local pixmap_properties = {
    IconPixmap          = true,
    AttentionIconPixmap = true,
}

-- Some applications register their item slightly before exporting it, so the
-- first load is attempted a few times before giving up.
local max_load_attempts = 3
local load_retry_delay = 1

local function variant_equal(a, b)
    if a == nil or b == nil then return a == b end
    return a:equal(b)
end

local function parse_tooltip(v)
    if not v or v.type ~= "(sa(iiay)ss)" then return nil end

    return {
        icon_name   = v:get_child_value(0).value,
        icon_pixmap = v:get_child_value(1),
        title       = v:get_child_value(2).value,
        text        = v:get_child_value(3).value,
    }
end

local function call(self, method, params, callback)
    local conn = self._private.connection

    conn:call(
        self._private.unique_name or self.service,
        self.path,
        self._private.interface,
        method,
        params,
        nil,
        Gio.DBusCallFlags.NONE,
        -1,
        nil,
        function(c, res)
            local ret, err = c:call_finish(res)
            if callback then
                protected_call(callback, ret, err)
            end
        end
    )
end

-- Apply a new set of raw properties (name -> GLib.Variant).
local function apply_properties(self, raw)
    local changed = false
    local old = self._private.raw

    for name, lua_name in pairs(property_names) do
        if not variant_equal(old[name], raw[name]) then
            local v = raw[name]
            local value = nil
            if v then value = v.value end
            self._private.values[lua_name] = value
            changed = true
            self:emit_signal("property::" .. lua_name, value)
        end
    end

    for name in pairs(pixmap_properties) do
        if not variant_equal(old[name], raw[name]) then
            changed = true
        end
    end

    if not variant_equal(old.ToolTip, raw.ToolTip) then
        local value = parse_tooltip(raw.ToolTip)
        self._private.values.tooltip = value
        changed = true
        self:emit_signal("property::tooltip", value)
    end

    if changed then
        self:_clear_icon_cache()
    end

    self._private.raw = raw

    return changed
end

local function load_properties_one_by_one(self, callback)
    local raw, pending = {}, 0

    local function done()
        pending = pending - 1
        if pending == 0 then callback(raw) end
    end

    local names = gtable.join(gtable.keys(property_names),
        gtable.keys(pixmap_properties), { "ToolTip" })

    pending = #names

    for _, name in ipairs(names) do
        self._private.connection:call(
            self._private.unique_name or self.service,
            self.path,
            "org.freedesktop.DBus.Properties",
            "Get",
            GLib.Variant("(ss)", { self._private.interface, name }),
            nil,
            Gio.DBusCallFlags.NONE,
            -1,
            nil,
            function(c, res)
                local ret = c:call_finish(res)
                if ret then
                    raw[name] = ret:get_child_value(0):get_variant()
                end
                done()
            end
        )
    end
end

-- The first load failed: nothing could be read. Retry a few times, then give
-- up and keep the item hidden. It stays registered, so it is removed normally
-- when the application goes away.
local function initial_load_failed(self)
    local priv = self._private

    priv.load_attempts = (priv.load_attempts or 0) + 1

    if priv.load_attempts < max_load_attempts then
        if priv.retry_queued then return end

        priv.retry_queued = true

        gtimer.start_new(load_retry_delay, function()
            priv.retry_queued = false
            self:refresh()
            return false
        end)
    elseif not priv.load_warned then
        priv.load_warned = true
        gdebug.print_warning("statusnotifier: Cannot read the properties of " ..
            self.service .. self.path .. ", ignoring the item")
    end
end

--- Reload all properties from the application.
-- @method refresh
-- @noreturn
function item:refresh()
    if self._private.removed then return end

    local getall_err

    local function finish(raw)
        if self._private.removed then return end

        if next(raw) == nil then
            -- Nothing could be read. Keep the current state, if any.
            if not self._private.loaded then
                initial_load_failed(self)
            end
            return
        end

        if getall_err and not self._private.getall_warned then
            -- Some implementations fail GetAll when one property fails.
            self._private.getall_warned = true
            gdebug.print_warning("statusnotifier: GetAll failed for " ..
                self.service .. self.path .. ": " .. tostring(getall_err))
        end

        local first = not self._private.loaded
        self._private.loaded = true

        if apply_properties(self, raw) or first then
            self:emit_signal("updated")
        end

        if first then
            self:emit_signal("ready")
        end
    end

    self._private.connection:call(
        self._private.unique_name or self.service,
        self.path,
        "org.freedesktop.DBus.Properties",
        "GetAll",
        GLib.Variant("(s)", { self._private.interface }),
        nil,
        Gio.DBusCallFlags.NONE,
        -1,
        nil,
        function(c, res)
            local ret, err = c:call_finish(res)

            if not ret then
                getall_err = err
                load_properties_one_by_one(self, finish)
                return
            end

            local raw = {}
            local dict = ret:get_child_value(0)

            for i = 0, dict:n_children() - 1 do
                local entry = dict:get_child_value(i)
                raw[entry:get_child_value(0).value] =
                    entry:get_child_value(1):get_variant()
            end

            finish(raw)
        end
    )
end

-- Coalesce the update signals, some applications send many at once.
function item:_queue_refresh()
    if self._private.refresh_queued then return end

    self._private.refresh_queued = true

    gtimer.delayed_call(function()
        self._private.refresh_queued = false
        self:refresh()
    end)
end

--- Get the status-dependent icon.
--
-- The returned table has either a `path` (an image file) or a `surface`
-- key. The `symbolic` key is true for symbolic icons, which should be
-- recolored.
--
-- @method get_icon
-- @tparam[opt=32] integer size The desired icon size in pixels.
-- @treturn table|nil The icon.
function item:get_icon(size)
    size = size or 32

    local cache = self._private.icon_cache

    if cache[size] ~= nil then return cache[size] or nil end

    local raw = self._private.raw
    local attention = self.status == "NeedsAttention"
    local theme_path = self.icon_theme_path

    local candidates = {}

    if attention then
        table.insert(candidates, { name = self.attention_icon_name })
        table.insert(candidates, { pixmap = raw.AttentionIconPixmap })
    end

    table.insert(candidates, { name = self.icon_name })
    table.insert(candidates, { pixmap = raw.IconPixmap })

    local ret

    for _, c in ipairs(candidates) do
        if c.name and c.name ~= "" then
            local path = icon.lookup(c.name, theme_path, size)
            if path then
                ret = { path = path, symbolic = icon.is_symbolic(path) }
                break
            end
        elseif c.pixmap then
            local surf = icon.pixmap_to_surface(c.pixmap, size)
            if surf then
                ret = { surface = surf }
                break
            end
        end
    end

    cache[size] = ret or false

    return ret
end

--- Forget the cached icons (for example after changing the icon theme).
-- @method _clear_icon_cache
-- @hidden
function item:_clear_icon_cache()
    self._private.icon_cache = {}
end

--- Get the item menu.
-- @method get_menu
-- @treturn wibox.widget.systray.dbusmenu|nil The menu, if the item has one.
function item:get_menu()
    local path = self.menu_path

    if (not path) or path == "" or path == "/" or path == "/NO_DBUSMENU" then
        return nil
    end

    local m = self._private.menu

    if m and m.path == path then return m end

    if m then m:destroy() end

    local dbusmenu = require("wibox.widget.systray.dbusmenu")

    m = dbusmenu(
        self._private.connection,
        self._private.unique_name or self.service,
        path,
        { icon_theme_path = self.icon_theme_path }
    )
    self._private.menu = m

    return m
end

local function coords(x, y)
    if not (x and y) then
        local c = capi.mouse.coords()
        x, y = x or c.x, y or c.y
    end
    return GLib.Variant("(ii)", { math.floor(x), math.floor(y) })
end

--- Primary action (usually a left click).
--
-- If the application does not implement the method, the menu is shown
-- instead (if any).
--
-- @method activate
-- @tparam[opt=mouse.coords().x] integer x
-- @tparam[opt=mouse.coords().y] integer y
-- @noreturn
function item:activate(x, y)
    if self.item_is_menu then
        self:show_menu(x, y, 1)
        return
    end

    call(self, "Activate", coords(x, y), function(_, err)
        if err then self:show_menu(x, y, 1) end
    end)
end

--- Secondary action (usually a middle click).
-- @method secondary_activate
-- @tparam[opt=mouse.coords().x] integer x
-- @tparam[opt=mouse.coords().y] integer y
-- @noreturn
function item:secondary_activate(x, y)
    call(self, "SecondaryActivate", coords(x, y))
end

--- Ask the application to show its own context menu.
-- @method context_menu
-- @tparam[opt=mouse.coords().x] integer x
-- @tparam[opt=mouse.coords().y] integer y
-- @noreturn
function item:context_menu(x, y)
    call(self, "ContextMenu", coords(x, y))
end

--- Show the menu (a `com.canonical.dbusmenu` menu when available, otherwise
-- the application context menu).
-- @method show_menu
-- @tparam[opt=mouse.coords().x] integer x
-- @tparam[opt=mouse.coords().y] integer y
-- @tparam[opt=3] integer button The mouse button which triggered the menu.
-- @noreturn
function item:show_menu(x, y, button)
    local m = self:get_menu()

    if not m then
        self:context_menu(x, y)
        return
    end

    m:show {
        coords   = (x and y) and { x = x, y = y } or nil,
        button   = button,
        fallback = function() self:context_menu(x, y) end,
    }
end

--- Scroll on the item.
-- @method scroll
-- @tparam integer delta The scroll amount.
-- @tparam[opt="vertical"] string orientation `vertical` or `horizontal`.
-- @noreturn
function item:scroll(delta, orientation)
    call(self, "Scroll",
        GLib.Variant("(is)", { math.floor(delta), orientation or "vertical" }))
end

--- Forget about this item.
-- @method _remove
-- @hidden
function item:_remove()
    if self._private.removed then return end

    self._private.removed = true

    if self._private.subscription then
        self._private.connection:signal_unsubscribe(self._private.subscription)
        self._private.subscription = nil
    end

    if self._private.menu then
        self._private.menu:destroy()
        self._private.menu = nil
    end

    self:emit_signal("removed")
end

local function new(_, args)
    local self = gobject {
        enable_properties = true,
    }

    rawset(self, "service", args.service)
    rawset(self, "path", args.path)

    self._private = {
        connection  = args.connection,
        interface   = args.prefix .. ".StatusNotifierItem",
        raw         = {},
        values      = {},
        icon_cache  = {},
    }

    -- Expose the D-Bus properties as read-only Lua properties.
    for _, lua_name in pairs(property_names) do
        rawset(self, "get_" .. lua_name, function(s)
            return s._private.values[lua_name]
        end)
    end

    rawset(self, "get_tooltip", function(s) return s._private.values.tooltip end)

    gtable.crush(self, item, true)

    local function subscribe()
        self._private.subscription = args.connection:signal_subscribe(
            self._private.unique_name,
            self._private.interface,
            nil,
            self.path,
            nil,
            Gio.DBusSignalFlags.NONE,
            function(_, _, _, _, signal_name)
                if self._private.removed then return end

                -- Every "New*" signal means some property changed.
                if signal_name:match("^New") then
                    protected_call(self._queue_refresh, self)
                end
            end
        )

        self:refresh()
    end

    -- Signals must be matched on the unique name. Otherwise, two items with
    -- the same (very common) path would receive each other's signals.
    if args.service:sub(1, 1) == ":" then
        self._private.unique_name = args.service
        subscribe()
    else
        args.connection:call(
            "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus", "GetNameOwner",
            GLib.Variant("(s)", { args.service }),
            nil, Gio.DBusCallFlags.NONE, -1, nil,
            function(c, res)
                local ret = c:call_finish(res)

                if self._private.removed then return end

                if ret then
                    self._private.unique_name = ret.value[1]
                end

                subscribe()
            end
        )
    end

    return self
end

return setmetatable(item, { __call = new })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
