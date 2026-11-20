---------------------------------------------------------------------------
-- A `com.canonical.dbusmenu` client displayed as a native GTK menu.
--
-- StatusNotifierItems usually export their context menu using this
-- protocol. The menu layout is fetched when the menu is shown and clicks are
-- sent back to the application. The menu is displayed with GTK (like the
-- menus of XEmbed tray icons and `snixembed`), so it uses the GTK theme and
-- supports keyboard navigation.
--
-- Known limitations:
--
-- * The menu is a snapshot of the layout taken when it is shown. The
--   `LayoutUpdated` and `ItemsPropertiesUpdated` signals are not followed, so
--   an open menu is not updated when the application changes it (for example
--   the state of a toggle or a label).
-- * Lazily populated submenus are only populated one level deep: submenus
--   which only appear once their parent is populated stay empty (see
--   `fetch`).
--
-- This is a private module.
--
-- @author WooParadog
-- @copyright 2026 WooParadog
-- @module wibox.widget.systray.dbusmenu
-- @supermodule gears.object
---------------------------------------------------------------------------

local lgi = require("lgi")
local Gio, GLib = lgi.Gio, lgi.GLib
local gobject = require("gears.object")
local gtable = require("gears.table")
local gdebug = require("gears.debug")
local gtimer = require("gears.timer")
local protected_call = require("gears.protected_call")
local icon = require("wibox.widget.systray.icon")

local capi = {
    awesome      = awesome,
    mouse        = mouse,
    mousegrabber = mousegrabber,
}

local dbusmenu = {}

local INTERFACE = "com.canonical.dbusmenu"

local defaults = {
    type               = "standard",
    label              = "",
    enabled            = true,
    visible            = true,
    ["icon-name"]      = "",
    ["toggle-type"]    = "",
    ["toggle-state"]   = -1,
    ["children-display"] = "",
}

--- Parse a `(ia{sv}av)` menu layout node.
-- @tparam GLib.Variant v The node.
-- @treturn table The node as `{ id = ..., props = {...}, children = {...} }`.
-- @staticfct wibox.widget.systray.dbusmenu.parse_layout
function dbusmenu.parse_layout(v)
    local node = {
        id       = math.floor(v:get_child_value(0).value),
        props    = setmetatable({}, { __index = defaults }),
        children = {},
    }

    local props = v:get_child_value(1)

    for i = 0, props:n_children() - 1 do
        local entry = props:get_child_value(i)
        local key = entry:get_child_value(0).value
        local val = entry:get_child_value(1):get_variant()

        if key == "icon-data" then
            node.props[key] = icon.variant_bytes(val)
        elseif not val:is_container() then
            -- The other used properties are basic types. Containers (like
            -- `shortcut`, an `aas`) are not used, and `.value` would return
            -- them as userdata.
            node.props[key] = val.value
        end
    end

    local children = v:get_child_value(2)

    for i = 0, children:n_children() - 1 do
        local child = children:get_child_value(i):get_variant()

        if child.type == "(ia{sv}av)" then
            table.insert(node.children, dbusmenu.parse_layout(child))
        end
    end

    return node
end

local function call(self, method, params, callback)
    self._private.connection:call(
        self.service, self.path, INTERFACE, method, params, nil,
        Gio.DBusCallFlags.NONE, -1, nil,
        function(c, res)
            local ret, err = c:call_finish(res)
            if callback then protected_call(callback, ret, err) end
        end
    )
end

--- Send an event to the application.
-- @method send_event
-- @tparam integer id The menu item id.
-- @tparam[opt="clicked"] string event The event name.
-- @noreturn
function dbusmenu:send_event(id, event)
    call(self, "Event", GLib.Variant("(isvu)", {
        id, event or "clicked", GLib.Variant("i", 0), os.time() % 4294967296
    }))
end

--- Fetch the menu layout.
--
-- `AboutToShow` is sent first, since some applications only populate their
-- menus on demand. It is then sent for every submenu which is still empty,
-- and the layout is fetched again if an application asks for it. Submenus
-- which are still empty in this second layout (for example the submenus of
-- a lazily populated submenu) are not populated.
--
-- @method fetch
-- @tparam function callback Called with the parsed root node (or nil).
-- @noreturn
function dbusmenu:fetch(callback)
    local function get_layout(cb)
        call(self, "GetLayout", GLib.Variant("(iias)", { 0, -1, {} }),
            function(ret, err)
                if not ret then
                    gdebug.print_warning("statusnotifier: GetLayout failed: " ..
                        tostring(err))
                    cb(nil)
                    return
                end

                cb(dbusmenu.parse_layout(ret:get_child_value(1)))
            end
        )
    end

    -- Lazily populated submenus are empty until `AboutToShow` is sent.
    local function populate(root)
        if not root then return callback(nil) end

        local empty = {}

        local function walk(node)
            for _, child in ipairs(node.children) do
                if child.props["children-display"] == "submenu" and
                  #child.children == 0 then
                    table.insert(empty, child.id)
                end
                walk(child)
            end
        end

        walk(root)

        if #empty == 0 then return callback(root) end

        local pending, need_update = #empty, false

        for _, id in ipairs(empty) do
            call(self, "AboutToShow", GLib.Variant("(i)", { id }), function(ret)
                if ret and ret.value[1] then need_update = true end

                pending = pending - 1

                if pending == 0 then
                    if need_update then
                        get_layout(callback)
                    else
                        callback(root)
                    end
                end
            end)
        end
    end

    -- The return value (whether an update is needed) is ignored since the
    -- layout is always fetched.
    call(self, "AboutToShow", GLib.Variant("(i)", { 0 }), function()
        get_layout(populate)
    end)
end

-- GTK is only loaded when a menu is shown for the first time.
local gtk

local function get_gtk()
    if gtk == nil then
        local ok, Gtk = pcall(lgi.require, "Gtk", "3.0")

        if ok and Gtk then
            local has_x11, GdkX11 = pcall(lgi.require, "GdkX11", "3.0")
            gtk = {
                Gtk       = Gtk,
                Gdk       = lgi.require("Gdk", "3.0"),
                GdkPixbuf = lgi.GdkPixbuf,
                GdkX11    = has_x11 and GdkX11 or nil,
            }
        else
            gdebug.print_warning("systray: GTK 3 is not available, the " ..
                "menus of the StatusNotifierItems cannot be displayed: " ..
                tostring(Gtk))
            gtk = false
        end
    end

    return gtk or nil
end

--- Can the menus be displayed?
--
-- This requires GTK 3 (and its GObject introspection data).
--
-- @staticfct wibox.widget.systray.dbusmenu.is_supported
-- @treturn boolean
function dbusmenu.is_supported()
    return get_gtk() ~= nil
end

-- Icon theme directories added to the GTK icon theme.
local added_theme_paths = {}

local function make_image(g, props, theme_path)
    local data = props["icon-data"]

    if data and data ~= "" then
        local ok, pixbuf = pcall(function()
            local loader = g.GdkPixbuf.PixbufLoader()
            loader:write(data)
            loader:close()
            return loader:get_pixbuf()
        end)

        if ok and pixbuf then
            return g.Gtk.Image.new_from_pixbuf(pixbuf)
        end
    end

    local name = props["icon-name"]

    if name and name ~= "" then
        if name:sub(1, 1) == "/" then
            return g.Gtk.Image.new_from_file(name)
        end

        if theme_path and theme_path ~= "" and not added_theme_paths[theme_path] then
            added_theme_paths[theme_path] = true
            g.Gtk.IconTheme.get_default():append_search_path(theme_path)
        end

        return g.Gtk.Image.new_from_icon_name(name, g.Gtk.IconSize.MENU)
    end
end

local build_menu

local function make_item(self, g, node, theme_path)
    local Gtk = g.Gtk
    local props = node.props
    local toggle = props["toggle-type"]
    local mi

    if toggle == "checkmark" or toggle == "radio" then
        mi = Gtk.CheckMenuItem.new_with_mnemonic(props.label)
        mi:set_draw_as_radio(toggle == "radio")

        local state = props["toggle-state"]

        if state == 0 or state == 1 then
            -- Note: this emits `activate`, so it must be done before the
            -- signal is connected.
            mi:set_active(state == 1)
        else
            mi:set_inconsistent(true)
        end
    else
        local img = make_image(g, props, theme_path)

        if img then
            mi = Gtk.ImageMenuItem.new_with_mnemonic(props.label)
            mi:set_image(img)
            mi:set_always_show_image(true)
        else
            mi = Gtk.MenuItem.new_with_mnemonic(props.label)
        end
    end

    mi:set_sensitive(props.enabled and true or false)

    if props["children-display"] == "submenu" then
        mi:set_submenu(build_menu(self, g, node, theme_path))
    else
        local id = node.id

        function mi.on_activate()
            protected_call(self.send_event, self, id, "clicked")
        end
    end

    return mi
end

build_menu = function(self, g, node, theme_path)
    local menu = g.Gtk.Menu()
    local last_is_separator, count = true, 0

    for _, child in ipairs(node.children) do
        local props = child.props

        if props.visible then
            if props.type == "separator" then
                -- Skip leading and repeated separators.
                if not last_is_separator then
                    menu:append(g.Gtk.SeparatorMenuItem())
                    last_is_separator = true
                end
            else
                menu:append(make_item(self, g, child, theme_path))
                last_is_separator = false
                count = count + 1
            end
        end
    end

    -- Remove the trailing separator.
    local children = menu:get_children()
    local last = children[#children]

    if last and last_is_separator then
        menu:remove(last)
    end

    menu:show_all()

    return menu, count
end

--- Build a GTK menu from a parsed layout.
-- @method build
-- @tparam table root The root node (see `parse_layout`).
-- @treturn Gtk.Menu|nil The menu.
function dbusmenu:build(root)
    local g = get_gtk()

    if not g then return nil end

    local menu, count = build_menu(self, g, root, self._private.theme_path)

    if count == 0 then
        menu:destroy()
        return nil
    end

    return menu
end

-- Get a timestamp for the GTK grabs. GTK needs it to know if the button
-- release which follows the click must close the menu.
local function get_time(g, menu)
    if not g.GdkX11 then return 0 end

    local ok, time = pcall(function()
        local toplevel = menu:get_toplevel()
        toplevel:realize()
        return g.GdkX11.x11_get_server_time(toplevel:get_window())
    end)

    return ok and time or 0
end

local function popup(g, menu, coords, button)
    local Gdk = g.Gdk

    -- While a mouse button is pressed over a wibox, awesome's X11 connection
    -- holds the implicit pointer grab of the button press. GTK uses its own
    -- connection, so it cannot grab the pointer and `popup_at_rect` would not
    -- show the menu.
    --
    -- `mousegrabber.stop()` is only used for its side effect: it calls
    -- `xcb_ungrab_pointer`, which also releases the implicit grab.
    -- `awesome.sync()` makes sure the X server processed the ungrab before GTK
    -- tries to grab the pointer.
    --
    -- When a Lua mousegrabber is running, it owns the grab and must not be
    -- stopped. Nothing is ungrabbed, so the GTK grab fails and the menu is
    -- not shown (see `show`).
    if not capi.mousegrabber.isrunning() then
        capi.mousegrabber.stop()
        capi.awesome.sync()
    end

    local root = Gdk.get_default_root_window()
    local event = Gdk.Event.new(Gdk.EventType.BUTTON_PRESS)
    event.button.window = root
    event.button.time = get_time(g, menu)
    event.button.button = button or 3
    event.button.x, event.button.y = coords.x, coords.y
    event.button.x_root, event.button.y_root = coords.x, coords.y

    local display = Gdk.Display.get_default()
    local seat = display and display:get_default_seat()

    if seat then
        event:set_device(seat:get_pointer())
    end

    menu:popup_at_rect(
        root,
        Gdk.Rectangle { x = coords.x, y = coords.y, width = 1, height = 1 },
        Gdk.Gravity.NORTH_WEST,
        Gdk.Gravity.NORTH_WEST,
        event
    )
end

--- Is the menu visible?
-- @method is_visible
-- @treturn boolean
function dbusmenu:is_visible()
    local m = self._private.menu
    return m and m:get_visible() or false
end

--- Hide the menu.
-- @method hide
-- @noreturn
function dbusmenu:hide()
    local m = self._private.menu

    if m then
        self._private.menu = nil
        m:popdown()
        m:destroy()
    end
end

--- Fetch the layout and show the menu.
-- @method show
-- @tparam[opt={}] table args
-- @tparam[opt=mouse.coords()] table args.coords The menu position.
-- @tparam[opt=3] integer args.button The mouse button used to show the menu.
-- @tparam[opt] function args.fallback Called when the menu cannot be shown.
-- @noreturn
function dbusmenu:show(args)
    args = args or {}

    local g = get_gtk()

    if not g then
        if args.fallback then args.fallback() end
        return
    end

    local coords = args.coords or capi.mouse.coords()

    -- Ignore results from older requests.
    self._private.request = (self._private.request or 0) + 1
    local request = self._private.request

    self:fetch(function(root)
        if self._private.destroyed or request ~= self._private.request then return end

        self:hide()

        local m = root and self:build(root)

        if not m then
            if args.fallback then args.fallback() end
            return
        end

        self._private.menu = m

        function m.on_deactivate()
            -- The item is activated after the menu is deactivated, so
            -- destroy the menu later.
            gtimer.delayed_call(function()
                if self._private.menu == m then
                    self:hide()
                end
                self:send_event(0, "closed")
            end)
        end

        self:send_event(0, "opened")
        popup(g, m, coords, args.button)

        -- GTK does not show the menu (and does not emit `deactivate`) when it
        -- cannot grab the pointer and the keyboard.
        if not m:get_visible() then
            gdebug.print_warning("systray: the menu could not be shown, " ..
                "the pointer or the keyboard is grabbed")
            self:hide()
            self:send_event(0, "closed")
            return
        end

        self:emit_signal("shown", m)
    end)
end

--- Stop using this menu.
-- @method destroy
-- @noreturn
function dbusmenu:destroy()
    self:hide()
    self._private.destroyed = true
end

local function new(_, connection, service, path, args)
    local self = gobject { enable_properties = false }

    args = args or {}

    rawset(self, "service", service)
    rawset(self, "path", path)

    self._private = {
        connection = connection,
        theme_path = args.icon_theme_path,
    }

    gtable.crush(self, dbusmenu, true)

    return self
end

return setmetatable(dbusmenu, { __call = new })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
