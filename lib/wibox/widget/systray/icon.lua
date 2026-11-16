---------------------------------------------------------------------------
-- Icon helpers for the StatusNotifierItem implementation.
--
-- This module converts the various icon representations used by
-- StatusNotifierItem and DBusMenu into something awesome can display:
--
-- * `a(iiay)` pixmaps (non-premultiplied ARGB32 in network byte order),
-- * freedesktop icon names, optionally with an application specific theme
--   path.
--
-- Note that `lgi` truncates `ay` values at the first NUL byte when using
-- `GLib.Variant.value`. All binary data is thus extracted using
-- `get_data_as_bytes()`.
--
-- @author WooParadog
-- @copyright 2026 WooParadog
-- @module wibox.widget.systray.icon
---------------------------------------------------------------------------

local lgi = require("lgi")
local GLib = lgi.GLib
local cairo = lgi.cairo
local gfs = require("gears.filesystem")
local gsurface = require("gears.surface")
local gdebug = require("gears.debug")
local beautiful = require("beautiful")

local sbyte, schar, tconcat, floor = string.byte, string.char, table.concat, math.floor

local module = {}

-- cairo's ARGB32 is stored in native endianness.
local little_endian = true
local spack = rawget(string, "pack")
if spack then
    little_endian = spack("=I4", 1):byte(1) == 1
end

-- Pre-compute the premultiplication table to make conversions faster.
local premul = {}
for a = 0, 255 do
    local row = {}
    for c = 0, 255 do
        row[c] = floor(c * a / 255 + 0.5)
    end
    premul[a] = row
end

--- Extract the raw bytes of a `GLib.Variant` of type `ay`.
-- @tparam GLib.Variant v The variant.
-- @treturn string The raw bytes (may contain NUL bytes).
function module.variant_bytes(v)
    if not v then return "" end
    local bytes = v:get_data_as_bytes()
    return bytes and bytes:get_data() or ""
end

--- Convert a single non-premultiplied big-endian ARGB32 pixmap to a surface.
-- @tparam integer w The width.
-- @tparam integer h The height.
-- @tparam string data The pixel data.
-- @treturn cairo.ImageSurface|nil The surface.
function module.argb_to_surface(w, h, data)
    w, h = floor(w), floor(h)
    if w <= 0 or h <= 0 or #data < w * h * 4 then return nil end

    local stride = cairo.Format.stride_for_width(cairo.Format.ARGB32, w)
    local pad = schar(0):rep(stride - 4 * w)
    local rows = {}

    for y = 0, h - 1 do
        local row = {}
        local offset = y * w * 4

        for x = 0, w - 1 do
            local i = offset + x * 4 + 1
            local a, r, g, b = sbyte(data, i, i + 3)

            if a ~= 255 then
                local p = premul[a]
                r, g, b = p[r], p[g], p[b]
            end

            if little_endian then
                row[x + 1] = schar(b, g, r, a)
            else
                row[x + 1] = schar(a, r, g, b)
            end
        end

        row[w + 1] = pad
        rows[y + 1] = tconcat(row)
    end

    local pixels = tconcat(rows)
    local surf = cairo.ImageSurface.create_for_data(
        pixels, cairo.Format.ARGB32, w, h, stride
    )

    -- The surface refers to `pixels`, which belongs to the Lua GC. Copy it.
    local ret = gsurface.duplicate_surface(surf)
    surf:finish()

    return ret
end

--- Pick the best pixmap from an `a(iiay)` variant and convert it.
--
-- The smallest pixmap at least `size` pixels wide is used. If there is none,
-- the biggest one is used.
--
-- @tparam GLib.Variant v The `a(iiay)` variant.
-- @tparam[opt=32] integer size The desired size.
-- @treturn cairo.ImageSurface|nil The surface.
function module.pixmap_to_surface(v, size)
    if not v or v.type ~= "a(iiay)" then return nil end

    size = size or 32
    local best, best_w

    for i = 0, v:n_children() - 1 do
        local child = v:get_child_value(i)
        local w = child:get_child_value(0).value
        local h = child:get_child_value(1).value

        if w > 0 and h > 0 then
            local better = not best
                or (best_w < size and w > best_w)
                or (w >= size and w < best_w)

            if better then
                best, best_w = child, w
            end
        end
    end

    if not best then return nil end

    return module.argb_to_surface(
        best:get_child_value(0).value,
        best:get_child_value(1).value,
        module.variant_bytes(best:get_child_value(2))
    )
end

-- Icon theme lookup -------------------------------------------------------

local extensions = { "png", "svg", "xpm" }

local function readable(path)
    return path and gfs.file_readable(path)
end

local base_dirs_cache

-- The directories in which icon themes are searched.
local function base_dirs()
    if base_dirs_cache then return base_dirs_cache end

    base_dirs_cache = {}

    local function add(d)
        if d and gfs.dir_readable(d) then
            table.insert(base_dirs_cache, d)
        end
    end

    add(GLib.build_filenamev { GLib.get_home_dir(), ".icons" })
    add(GLib.build_filenamev { GLib.get_user_data_dir(), "icons" })

    for _, d in ipairs(GLib.get_system_data_dirs()) do
        add(GLib.build_filenamev { d, "icons" })
    end

    return base_dirs_cache
end

local theme_cache = {}

-- A minimal parser for the (INI-like) desktop entry format. GLib.KeyFile
-- is not used because its lgi bindings do not report missing keys and
-- mishandle string lists.
local function parse_ini(path)
    local f = io.open(path, "r")

    if not f then return nil end

    local groups, current = {}, nil

    for line in f:lines() do
        local group = line:match("^%s*%[(.-)%]%s*$")

        if group then
            current = groups[group] or {}
            groups[group] = current
        elseif current and not line:match("^%s*#") then
            local key, value = line:match("^%s*([^=]-)%s*=%s*(.-)%s*$")
            if key and key ~= "" then
                current[key] = value
            end
        end
    end

    f:close()

    return groups
end

local function split_list(value)
    local ret = {}

    for v in (value or ""):gmatch("[^,;]+") do
        v = v:match("^%s*(.-)%s*$")
        if v ~= "" then table.insert(ret, v) end
    end

    return ret
end

-- Parse the `index.theme` of an icon theme.
local function load_theme(name)
    if theme_cache[name] ~= nil then return theme_cache[name] end

    local theme = { name = name, roots = {}, dirs = {}, inherits = {} }
    local index

    for _, base in ipairs(base_dirs()) do
        local root = GLib.build_filenamev { base, name }

        if gfs.dir_readable(root) then
            table.insert(theme.roots, root)

            local path = GLib.build_filenamev { root, "index.theme" }

            if (not index) and readable(path) then
                index = parse_ini(path)
            end
        end
    end

    local main = index and index["Icon Theme"]

    if not main then
        theme_cache[name] = false
        return false
    end

    local dirs = split_list(main.Directories)

    for _, d in ipairs(split_list(main.ScaledDirectories)) do
        table.insert(dirs, d)
    end

    for _, d in ipairs(dirs) do
        local group = index[d]
        local size = group and tonumber(group.Size)

        if size then
            table.insert(theme.dirs, {
                path      = d,
                size      = size,
                scale     = tonumber(group.Scale) or 1,
                type      = group.Type or "Threshold",
                min_size  = tonumber(group.MinSize) or size,
                max_size  = tonumber(group.MaxSize) or size,
                threshold = tonumber(group.Threshold) or 2,
            })
        end
    end

    theme.inherits = split_list(main.Inherits)

    theme_cache[name] = theme

    return theme
end

-- See the freedesktop icon theme specification, "DirectorySizeDistance".
local function size_distance(dir, size)
    local s = size

    if dir.type == "Fixed" then
        return math.abs(dir.size * dir.scale - s)
    elseif dir.type == "Scalable" then
        if s < dir.min_size * dir.scale then
            return dir.min_size * dir.scale - s
        elseif s > dir.max_size * dir.scale then
            return s - dir.max_size * dir.scale
        end
        return 0
    else
        if s < (dir.size - dir.threshold) * dir.scale then
            return (dir.size - dir.threshold) * dir.scale - s
        elseif s > (dir.size + dir.threshold) * dir.scale then
            return s - (dir.size + dir.threshold) * dir.scale
        end
        return 0
    end
end

local function find_in_dir(dir, name)
    for _, ext in ipairs(extensions) do
        local path = GLib.build_filenamev { dir, name .. "." .. ext }
        if readable(path) then return path end
    end
end

local function lookup_in_theme(theme, name, size)
    local best, best_dist = nil, math.huge

    for _, dir in ipairs(theme.dirs) do
        for _, root in ipairs(theme.roots) do
            local path = find_in_dir(GLib.build_filenamev { root, dir.path }, name)

            if path then
                local dist = size_distance(dir, size)

                -- Prefer SVG for equal distances, they scale better.
                if dist < best_dist or (dist == best_dist and path:match("%.svg$")) then
                    best, best_dist = path, dist
                end
            end
        end
    end

    return best
end

local function lookup_in_chain(theme_name, name, size, visited)
    if visited[theme_name] then return nil end
    visited[theme_name] = true

    local theme = load_theme(theme_name)

    if not theme then return nil end

    local ret = lookup_in_theme(theme, name, size)

    if ret then return ret end

    for _, parent in ipairs(theme.inherits) do
        ret = lookup_in_chain(parent, name, size, visited)
        if ret then return ret end
    end
end

local gtk_theme_name

-- Get the icon theme name from the GTK settings, since this is what most
-- users configure.
local function get_gtk_theme_name()
    if gtk_theme_name ~= nil then return gtk_theme_name or nil end

    gtk_theme_name = false

    for _, version in ipairs { "gtk-4.0", "gtk-3.0" } do
        local path = GLib.build_filenamev {
            GLib.get_user_config_dir(), version, "settings.ini"
        }

        local ini = readable(path) and parse_ini(path)
        local name = ini and ini.Settings and ini.Settings["gtk-icon-theme-name"]

        if name and name ~= "" then
            gtk_theme_name = name:gsub('^"(.*)"$', "%1")
            return gtk_theme_name
        end
    end

    return nil
end

--- Get the icon theme which is used for lookups.
--
-- It is `beautiful.systray_icon_theme`, then `beautiful.icon_theme`,
-- then the GTK icon theme (from the `gtk-4.0` and then the `gtk-3.0`
-- `settings.ini` in the user config directory) and finally `Adwaita` (the
-- GTK default). The `hicolor` theme is always used as the last fallback.
-- @treturn string The theme name.
function module.get_theme_name()
    return beautiful.systray_icon_theme
        or beautiful.icon_theme
        or get_gtk_theme_name()
        or "Adwaita"
end

local lookup_cache = {}

local function lookup_uncached(name, theme_path, size, theme_name)
    local ret

    -- The application specific path is either a flat directory or a theme.
    if theme_path and theme_path ~= "" then
        ret = find_in_dir(theme_path, name)

        if not ret and gfs.dir_readable(theme_path) then
            -- Look for a theme-like structure (hicolor/22x22/apps/name.png).
            local best, best_dist = nil, math.huge
            local ok, dir = pcall(GLib.Dir.open, theme_path, 0)
            local subdirs = {}

            if ok and dir then
                for entry in function() return dir:read_name() end do
                    table.insert(subdirs, entry)
                end
                dir:close()
            end

            for _, theme_dir in ipairs(subdirs) do
                local tdir = GLib.build_filenamev { theme_path, theme_dir }
                local ok2, d2 = pcall(GLib.Dir.open, tdir, 0)
                if ok2 and d2 then
                    for size_dir in function() return d2:read_name() end do
                        local s = tonumber(size_dir:match("^(%d+)x%d+")) or
                            (size_dir == "scalable" and size) or nil
                        if s then
                            for _, ctx in ipairs { "apps", "status", "devices", "actions", "panel", "" } do
                                local path = find_in_dir(
                                    GLib.build_filenamev { tdir, size_dir, ctx }, name
                                )
                                if path then
                                    local dist = math.abs(s - size)
                                    if dist < best_dist then
                                        best, best_dist = path, dist
                                    end
                                end
                            end
                        end
                    end
                    d2:close()
                end
            end

            ret = best
        end
    end

    if not ret then
        local visited = {}
        ret = lookup_in_chain(theme_name, name, size, visited)
            or lookup_in_chain("hicolor", name, size, visited)
    end

    if not ret then
        for _, base in ipairs(base_dirs()) do
            ret = find_in_dir(base, name)
            if ret then break end
        end
    end

    if not ret then
        for _, d in ipairs(GLib.get_system_data_dirs()) do
            ret = find_in_dir(GLib.build_filenamev { d, "pixmaps" }, name)
            if ret then break end
        end
    end

    return ret
end

--- Find an icon file by name.
--
-- Like GTK, when an icon is not found, the symbolic (or the non-symbolic)
-- variant is also tried.
--
-- @tparam string name The icon name or an absolute path.
-- @tparam[opt] string theme_path An application specific icon theme path.
-- @tparam[opt=32] integer size The desired size.
-- @treturn string|nil The path to the file.
function module.lookup(name, theme_path, size)
    if (not name) or name == "" then return nil end

    size = size or 32

    if name:sub(1, 1) == "/" then
        return readable(name) and name or nil
    end

    local theme_name = module.get_theme_name()
    local key = tconcat({theme_name, theme_path or "", name, size}, "\0")

    if lookup_cache[key] ~= nil then return lookup_cache[key] or nil end

    local ret = lookup_uncached(name, theme_path, size, theme_name)

    if not ret then
        local alt = module.is_symbolic(name) and name:gsub("%-symbolic$", "")
            or name .. "-symbolic"
        ret = lookup_uncached(alt, theme_path, size, theme_name)
    end

    if not ret then
        gdebug.print_warning("statusnotifier: Icon '" .. name .. "' not found")
    end

    lookup_cache[key] = ret or false

    return ret
end

--- Clear the lookup caches (for example after changing the icon theme).
function module.clear_cache()
    lookup_cache = {}
    theme_cache = {}
    gtk_theme_name = nil
    base_dirs_cache = nil
end

--- Is the icon a symbolic icon (which should be recolored)?
-- @tparam string name The icon name or file path.
-- @treturn boolean
function module.is_symbolic(name)
    if type(name) ~= "string" then return false end
    return name:match("%-symbolic$") ~= nil
        or name:match("%-symbolic%.svg$") ~= nil
end

--- Create a stylesheet to recolor symbolic SVG icons like GTK does.
-- @tparam string color The foreground color (as `#rrggbb`).
-- @treturn string The stylesheet.
function module.symbolic_stylesheet(color)
    return string.format(
        "rect,circle,path,.foreground-fill { fill: %s !important; }" ..
        ".foreground-stroke { stroke: %s !important; }",
        color, color
    )
end

return module

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
