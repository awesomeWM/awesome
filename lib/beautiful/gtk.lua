---------------------------------------------------------------------------
--- Querying current GTK+ 3 theme via GtkStyleContext.
--
-- @author Yauheni Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2016-2017 Yauheni Kirylau
-- @module beautiful.gtk
---------------------------------------------------------------------------
local get_dpi = require("beautiful.xresources").get_dpi
local gears_debug = require("gears.debug")
local gears_math = require("gears.math")
local join = require("gears.table").join
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)


local gtk = {
    cached_theme_variables = nil
}


local function convert_gtk_channel_to_hex(channel_value)
    return string.format("%02x", gears_math.round(channel_value * 255))
end

local function convert_gtk_color_to_hex(gtk_color)
    return "#" ..
        convert_gtk_channel_to_hex(gtk_color.red) ..
        convert_gtk_channel_to_hex(gtk_color.green) ..
        convert_gtk_channel_to_hex(gtk_color.blue) ..
        convert_gtk_channel_to_hex(gtk_color.alpha)
end

local function lookup_gtk_color_to_hex(_style_context, color_name)
    local gtk_color = _style_context:lookup_color(color_name)
    if not gtk_color then
        return nil
    end
    return convert_gtk_color_to_hex(gtk_color)
end

local function get_gtk_property(_style_context, property_name)
    local state = _style_context:get_state()
    local property = _style_context:get_property(property_name, state)
    if not property then
        return nil
    end
    return property.value
end

local function get_gtk_color_property_to_hex(_style_context, property_name)
    return convert_gtk_color_to_hex(
        get_gtk_property(_style_context, property_name)
    )
end

local function read_gtk_color_properties_from_widget(gtk_widget, properties)
    local _style_context = gtk_widget:get_style_context()
    local result = {}
    for result_key, style_context_property in pairs(properties) do
        result[result_key] = get_gtk_color_property_to_hex(
            _style_context, style_context_property
        )
    end
    return result
end


-- luacheck: max comment line length 300

--- Get GTK+3 theme variables from GtkStyleContext
-- @treturn table Key-value table with the following structure:
-- <table class='widget_list' border=1>
-- <tr style='font-weight: bold;'> <th align='center'>Result key</th> <th align='center'>StyleContext key</th> <th align='center'>StyleContext fallback #1</th> <th align='center'>StyleContext fallback #2</th> <th align='center'>GTK Widget fallback</th> </tr>
-- <tr> <td>`font_size`</td> <td></td> <td></td> <td></td> <td>Label font-size</td> </tr>
-- <tr> <td>`font_family`</td> <td></td> <td></td> <td></td> <td>Label font-family</td> </tr>
-- <tr> <td>`bg_color`</td> <td>`theme_bg_color`</td> <td></td> <td></td> <td>Window bg</td> </tr>
-- <tr> <td>`fg_color`</td> <td>`theme_fg_color`</td> <td></td> <td></td> <td>Window fg</td> </tr>
-- <tr> <td>`base_color`</td> <td>`theme_base_color`</td> <td></td> <td></td> <td>Entry bg</td> </tr>
-- <tr> <td>`text_color`</td> <td>`theme_text_color`</td> <td></td> <td></td> <td>Entry fg</td> </tr>
-- <tr> <td>`button_bg_color`</td> <td>`theme_button_bg_color`</td> <td>`theme_bg_color`</td> <td></td> <td>Button bg</td> </tr>
-- <tr> <td>`button_fg_color`</td> <td>`theme_button_fg_color`</td> <td>`theme_fg_color`</td> <td></td> <td>Button fg</td> </tr>
-- <tr> <td>`button_border_color`</td> <td></td> <td></td> <td></td> <td>Button border-color</td> </tr>
-- <tr> <td>`button_border_radius`</td> <td></td> <td></td> <td></td> <td>Button border-radius</td> </tr>
-- <tr> <td>`button_border_width`</td> <td></td> <td></td> <td></td> <td>Button border-top-width</td> </tr>
-- <tr> <td>`selected_bg_color`</td> <td>`theme_selected_bg_color`</td> <td></td> <td></td> <td>ToggleButton bg</td> </tr>
-- <tr> <td>`selected_fg_color`</td> <td>`theme_selected_fg_color`</td> <td></td> <td></td> <td>ToggleButton fg</td> </tr>
-- <tr> <td>`menubar_bg_color`</td> <td>`menubar_bg_color`</td> <td>`theme_bg_color`</td> <td></td> <td>HeaderBar bg</td> </tr>
-- <tr> <td>`menubar_fg_color`</td> <td>`menubar_fg_color`</td> <td>`theme_fg_color`</td> <td></td> <td>HeaderBar fg</td> </tr>
-- <tr> <td>`header_button_bg_color`</td> <td>`header_button_bg_color`</td> <td>`menubar_bg_color`</td> <td>`theme_bg_color`</td> <td>HeaderBar > Button bg</td> </tr>
-- <tr> <td>`header_button_fg_color`</td> <td>`header_button_fg_color`</td> <td>`menubar_fg_color`</td> <td>`theme_fg_color`</td> <td>HeaderBar > Button fg</td> </tr>
-- <tr> <td>`header_button_border_color`</td> <td></td> <td></td> <td></td> <td>HeaderBar > Button border-color</td> </tr>
-- <tr> <td>`error_color`</td> <td>`error_color`</td> <td>`error_bg_color`</td> <td></td> <td>destructive Button bg</td> </tr>
-- <tr> <td>`error_bg_color`</td> <td>`error_bg_color`</td> <td>`error_color`</td> <td></td> <td>destructive Button bg</td> </tr>
-- <tr> <td>`error_fg_color`</td> <td>`error_fg_color`</td> <td>`theme_selected_fg_color`</td> <td></td> <td>destructive Button fg</td> </tr>
-- <tr> <td>`warning_color`</td> <td>`warning_color`</td> <td>`warning_bg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`warning_bg_color`</td> <td>`warning_bg_color`</td> <td>`warning_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`warning_fg_color`</td> <td>`warning_fg_color`</td> <td>`theme_selected_fg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`success_color`</td> <td>`success_color`</td> <td>`success_bg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`success_bg_color`</td> <td>`success_bg_color`</td> <td>`success_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`success_fg_color`</td> <td>`success_fg_color`</td> <td>`theme_selected_fg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`tooltip_bg_color`</td> <td>`theme_tooltip_bg_color`</td> <td>`theme_bg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`tooltip_fg_color`</td> <td>`theme_tooltip_fg_color`</td> <td>`theme_fg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`osd_bg_color`</td> <td>`osd_bg`</td> <td>`theme_tooltip_bg_color`</td> <td>`theme_bg_color`</td> <td></td> </tr>
-- <tr> <td>`osd_fg_color`</td> <td>`osd_fg`</td> <td>`theme_tooltip_fg_color`</td> <td>`theme_fg_color`</td> <td></td> </tr>
-- <tr> <td>`osd_border_color`</td> <td>`osd_borders_color`</td> <td>`osd_fg_color`</td> <td></td> <td></td> </tr>
-- <tr> <td>`wm_bg_color`</td> <td>`wm_bg`</td> <td>`menubar_bg_color`</td> <td>`theme_bg_color`</td> <td>HeaderBar bg</td> </tr>
-- <tr> <td>`wm_border_focused_color`</td> <td>`wm_border_focused`</td> <td>`theme_selected_bg_color`</td> <td></td> <td>ToggleButton bg</td> </tr>
-- <tr> <td>`wm_border_unfocused_color`</td> <td>`wm_border_unfocused`</td> <td>`wm_border`</td> <td>`menubar_bg_color`</td> <!--<td>`theme_bg_color`</td>--> <td>HeaderBar bg</td> </tr>
-- <tr> <td>`wm_title_focused_color`</td> <td>`wm_title_focused`</td> <td>`wm_title`</td> <td>`theme_selected_fg_color`</td> <td>ToggleButton fg</td> </tr>
-- <tr> <td>`wm_title_unfocused_color`</td> <td>`wm_title_unfocused`</td> <td>`wm_unfocused_title`</td> <td>`menubar_fg_color`</td> <!--<td>`theme_fg_color`</td>--> <td>HeaderBar fg</td> </tr>
-- <tr> <td>`wm_icons_focused_color`</td> <td>`wm_icons_focused`</td> <td>`wm_title_focused`</td> <td>`theme_selected_fg_color`</td> <td>ToggleButton fg</td> </tr>
-- <tr> <td>`wm_icons_unfocused_color`</td> <td>`wm_icons_unfocused`</td> <td>`wm_title_unfocused`</td> <td>`menubar_fg_color`</td> <!--<td>`theme_fg_color`</td>--> <td>HeaderBar fg</td> </tr>
-- </table>
--
function gtk.get_theme_variables()
    if gtk.cached_theme_variables then
        return gtk.cached_theme_variables
    end

    local result = {}
    local _gtk_status, Gtk = pcall(function()
        return require('lgi').Gtk
    end)
    if not _gtk_status or not Gtk then
        gears_debug.print_warning(
            "Can't load GTK+3 introspection. "..
            "Seems like GTK+3 is not installed or `lua-lgi` was built with an incompatible GTK+3 version."
        )
        return nil
    end
    local _window_status, window = pcall(function()
        return Gtk.Window.new(Gtk.WindowType.TOPLEVEL)
    end)
    if not _window_status or not window then
        gears_debug.print_warning(
            "Can't create GTK+3 window. "..
            "Seems like GTK+3 theme is not set correctly or `lua-lgi` was built with an incompatible GTK+3 version."
        )
        return nil
    end
    local style_context = window:get_style_context()

    result = join(result, read_gtk_color_properties_from_widget(
        window, {
            bg_color="background-color",
            fg_color="color",
        }
    ))
    local label = Gtk.Label()
    local label_style_context = label:get_style_context()
    local gdk_scale = tonumber(os.getenv("GDK_SCALE") or 1.0)
    local xrdb_scale = get_dpi() / 96
    local pt_to_px_ratio = 1+1/3
    result["font_size"] = get_gtk_property(
        label_style_context, "font-size"
    ) * gdk_scale / xrdb_scale / pt_to_px_ratio
    result["font_family"] = get_gtk_property(
        label_style_context, "font-family"
    )[1]

    result = join(result, read_gtk_color_properties_from_widget(
        Gtk.Entry(), {
            base_color="background-color",
            text_color="color",
        }
    ))
    result = join(result, read_gtk_color_properties_from_widget(
        Gtk.ToggleButton(), {
            selected_bg_color="background-color",
            selected_fg_color="color",
        }
    ))
    local button = Gtk.Button()
    result = join(result, read_gtk_color_properties_from_widget(
        button, {
            button_bg_color="background-color",
            button_fg_color="color",
            button_border_color="border-color",
        }
    ))
    local button_style_context = button:get_style_context()
    for result_key, style_context_property in pairs({
        button_border_radius="border-radius",
        button_border_width="border-top-width",
    }) do
        local state = style_context:get_state()
        local property = button_style_context:get_property(style_context_property, state)
        result[result_key] = property.value
        property:unset()
    end

    local headerbar = Gtk.HeaderBar()
    window:set_titlebar(headerbar)
    result = join(result, read_gtk_color_properties_from_widget(
        headerbar, {
            menubar_bg_color="background-color",
        }
    ))

    if result.menubar_bg_color and result.menubar_bg_color ~= "#00000000" then
        headerbar:add(label)
        result = join(result, read_gtk_color_properties_from_widget(
            label, {
                menubar_fg_color="color",
            }
        ))
    end

    headerbar:add(button)
    result = join(result, read_gtk_color_properties_from_widget(
        button, {
            header_button_bg_color="background-color",
            header_button_border_color="border-color",
        }
    ))
    if result.header_button_bg_color and result.header_button_bg_color ~= "#00000000" then
        result = join(result, read_gtk_color_properties_from_widget(
            button, {
                header_button_fg_color="color",
            }
        ))
    end

    local error_button = Gtk.Button()
    error_button:get_style_context():add_class("destructive-action")
    result = join(result, read_gtk_color_properties_from_widget(
        error_button, {
            error_bg_color="background-color",
            error_fg_color="color",
        }
    ))

    for _, color_data in ipairs({
        {"bg_color", "theme_bg_color"},
        {"fg_color", "theme_fg_color"},
        {"base_color", "theme_base_color"},
        {"text_color", "theme_text_color"},
        {"selected_bg_color", "theme_selected_bg_color"},
        {"selected_fg_color", "theme_selected_fg_color"},
        --
        {"tooltip_bg_color", "theme_tooltip_bg_color", "bg_color"},
        {"tooltip_fg_color", "theme_tooltip_fg_color", "fg_color"},
        {"osd_bg_color", "osd_bg", "tooltip_bg_color"},
        {"osd_fg_color", "osd_fg", "tooltip_fg_color"},
        {"osd_border_color", "osd_borders_color", "osd_fg_color"},
        {"menubar_bg_color", "menubar_bg_color", "bg_color"},
        {"menubar_fg_color", "menubar_fg_color", "fg_color"},
        --
        {"button_bg_color", "button_bg_color", "bg_color"},
        {"button_fg_color", "button_fg_color", "fg_color"},
        {"header_button_bg_color", "header_button_bg_color", "menubar_bg_color"},
        {"header_button_fg_color", "header_button_fg_color", "menubar_fg_color"},
        --
        {"wm_bg_color", "wm_bg", "menubar_bg_color"},
        {"wm_border_focused_color", "wm_border_focused", "selected_bg_color"},
        {"wm_border_unfocused_color", "wm_border_unfocused", "wm_border", "menubar_bg_color"},
        {"wm_title_focused_color", "wm_title_focused", "wm_title", "selected_fg_color"},
        {"wm_title_unfocused_color", "wm_title_unfocused", "wm_unfocused_title", "menubar_fg_color"},
        {"wm_icons_focused_color", "wm_icons_focused", "wm_title_focused_color", "selected_fg_color"},
        {"wm_icons_unfocused_color", "wm_icons_unfocused", "wm_title_unfocused_color", "menubar_fg_color"},
        --
        {"error_color", "error_color"},
        {"error_bg_color", "error_bg_color", "error_color"},
        {"error_fg_color", "error_fg_color", "selected_fg_color"},
        {"error_color", "error_color", "error_bg_color"},
        {"warning_color", "warning_color"},
        {"warning_bg_color", "warning_bg_color", "warning_color"},
        {"warning_fg_color", "warning_fg_color", "selected_fg_color"},
        {"warning_color", "warning_color", "warning_bg_color"},
        {"success_color", "success_color"},
        {"success_bg_color", "success_bg_color", "success_color"},
        {"success_fg_color", "success_fg_color", "selected_fg_color"},
        {"success_color", "success_color", "success_bg_color"},
    }) do
        local result_key, style_context_key, fallback_key, fallback_key2 = unpack(color_data)
        result[result_key] = lookup_gtk_color_to_hex(style_context, style_context_key) or (
            result[result_key] ~= "#00000000" and
            result[result_key] or
            result[fallback_key] or
            result[fallback_key2] or
            result[result_key]    -- <-- here is for case if it was meant to be a fully transparent color on purpose
        )
        if not result[result_key] then
            gears_debug.print_warning("Can't read color '" .. style_context_key .. "' from GTK+3 theme.")
        end
    end

    window:destroy()
    gtk.cached_theme_variables = result
    return result
end


return gtk
