---------------------------------------------------------------------------
--- Popup widget which shows current hotkeys and their descriptions.
--
-- @author Yauheni Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2014-2015 Yauheni Kirylau
-- @module awful.hotkeys_popup.widget
---------------------------------------------------------------------------

local capi = {
    screen = screen,
    client = client,
    keygrabber = keygrabber,
}
local awful = require("awful")
local wibox = require("wibox")
local beautiful = require("beautiful")
local dpi = beautiful.xresources.apply_dpi


-- Stripped copy of this module https://github.com/copycat-killer/lain/blob/master/util/markup.lua:
local markup = {}
-- Set the font.
function markup.font(font, text)
    return '<span font="'    .. tostring(font)    .. '">' .. tostring(text) ..'</span>'
end
-- Set the foreground.
function markup.fg(color, text)
    return '<span foreground="' .. tostring(color) .. '">' .. tostring(text) .. '</span>'
end
-- Set the background.
function markup.bg(color, text)
    return '<span background="' .. tostring(color) .. '">' .. tostring(text) .. '</span>'
end

local widget = {
    group_rules = {},
}

--- Don't show hotkeys without descriptions.
widget.hide_without_description = true

--- Merge hotkey records into one if they have the same modifiers and
-- description.
widget.merge_duplicates = true


--- Create an instance of widget with hotkeys help.
-- @return widget instance.
function widget.new()
    local widget_instance = {
        hide_without_description = widget.hide_without_description,
        merge_duplicates = widget.merge_duplicates,
        group_rules = awful.util.table.clone(widget.group_rules),
        title_font = "Monospace Bold 9",
        description_font = "Monospace 8",
        width = dpi(1200),
        height = dpi(800),
        border_width = beautiful.border_width or dpi(2),
        modifiers_color = beautiful.bg_minimize or "#555555",
        group_margin = dpi(6),
        additional_hotkeys = {},
        labels = {
            Mod4="Super",
            Mod1="Alt",
            Escape="Esc",
            Insert="Ins",
            Delete="Del",
            Backspace="BackSpc",
            Return="Enter",
            Next="PgDn",
            Prior="PgUp",
            ['#108']="Alt Gr",
            Left='←',
            Up='↑',
            Right='→',
            Down='↓',
            ['#67']="F1",
            ['#68']="F2",
            ['#69']="F3",
            ['#70']="F4",
            ['#71']="F5",
            ['#72']="F6",
            ['#73']="F7",
            ['#74']="F8",
            ['#75']="F9",
            ['#76']="F10",
            ['#95']="F11",
            ['#96']="F12",
            ['#10']="1",
            ['#11']="2",
            ['#12']="3",
            ['#13']="4",
            ['#14']="5",
            ['#15']="6",
            ['#16']="7",
            ['#17']="8",
            ['#18']="9",
            ['#19']="0",
            ['#20']="-",
            ['#21']="=",
            Control="Ctrl"
        },
    }

    local cached_wiboxes = {}
    local cached_awful_keys = nil
    local colors_counter = {}
    local colors = beautiful.xresources.get_current_theme()
    local group_list = {}


    local function get_next_color(id)
        id = id or "default"
        if colors_counter[id] then
            colors_counter[id] = math.fmod(colors_counter[id] + 1, 15) + 1
        else
            colors_counter[id] = 1
        end
        return colors["color"..tostring(colors_counter[id], 15)]
    end


    local function join_plus_sort(modifiers)
        if #modifiers<1 then return "none" end
        table.sort(modifiers)
        return table.concat(modifiers, '+')
    end


    local function add_hotkey(key, data, target)
        if widget_instance.hide_without_description and not data.description then return end

        local readable_mods = {}
        for _, mod in ipairs(data.mod) do
            table.insert(readable_mods, widget_instance.labels[mod] or mod)
        end
        local joined_mods = join_plus_sort(readable_mods)

        local group = data.group or "none"
        group_list[group] = true
        if not target[group] then target[group] = {} end
        local new_key = {
            key = (widget_instance.labels[key] or key),
            mod = joined_mods,
            description = data.description
        }
        local index = data.description or "none"  -- or use its hash?
        if not target[group][index] then
            target[group][index] = new_key
        else
            if widget_instance.merge_duplicates and joined_mods == target[group][index].mod then
                target[group][index].key = target[group][index].key .. "/" .. new_key.key
            else
                while target[group][index] do
                    index = index .. " "
                end
                target[group][index] = new_key
            end
        end
    end


    local function sort_hotkeys(target)
        -- @TODO: add sort by 12345qwertyasdf etc
        for group, _ in pairs(group_list) do
            if target[group] then
                local sorted_table = {}
                for _, key in pairs(target[group]) do
                    table.insert(sorted_table, key)
                end
                table.sort(
                    sorted_table,
                    function(a,b) return (a.mod or '')..a.key<(b.mod or '')..b.key end
                )
                target[group] = sorted_table
            end
        end
    end


    local function import_awful_keys()
        if cached_awful_keys then
            return
        end
        cached_awful_keys = {}
        for _, data in pairs(awful.key.hotkeys) do
            add_hotkey(data.key, data, cached_awful_keys)
        end
        sort_hotkeys(cached_awful_keys)
    end


    local function group_label(group, color)
        local textbox = wibox.widget.textbox(
            markup.font(widget_instance.title_font,
                markup.bg(
                    color or (widget_instance.group_rules[group] and
                        widget_instance.group_rules[group].color or get_next_color("group_title")
                    ),
                    markup.fg(beautiful.bg_normal or "#000000", " "..group.." ")
                )
            )
        )
        local margin = wibox.container.margin()
        margin:set_widget(textbox)
        margin:set_top(widget_instance.group_margin)
        return margin
    end

    local function get_screen(s)
        return s and capi.screen[s]
    end

    local function create_wibox(s, available_groups)
        s = get_screen(s)

        local wa = s.workarea
        local height = (widget_instance.height < wa.height) and widget_instance.height or
            (wa.height - widget_instance.border_width * 2)
        local width = (widget_instance.width < wa.width) and widget_instance.width or
            (wa.width - widget_instance.border_width * 2)

        -- arrange hotkey groups into columns
        local line_height = beautiful.get_font_height(widget_instance.title_font)
        local group_label_height = line_height + widget_instance.group_margin
        -- -1 for possible pagination:
        local max_height_px = height - group_label_height
        local column_layouts = {}
        for _, group in ipairs(available_groups) do
            local keys = awful.util.table.join(cached_awful_keys[group], widget_instance.additional_hotkeys[group])
            local joined_descriptions = ""
            for i, key in ipairs(keys) do
                joined_descriptions = joined_descriptions .. key.description .. (i~=#keys and "\n" or "")
            end
            -- +1 for group label:
            local items_height = awful.util.linecount(joined_descriptions) * line_height + group_label_height
            local current_column
            local available_height_px = max_height_px
            local add_new_column = true
            for i, column in ipairs(column_layouts) do
                if ((column.height_px + items_height) < max_height_px) or
                    (i == #column_layouts and column.height_px < max_height_px / 2)
                then
                    current_column = column
                    add_new_column = false
                    available_height_px = max_height_px - current_column.height_px
                    break
                end
            end
            local overlap_leftovers
            if items_height > available_height_px then
                local new_keys = {}
                overlap_leftovers = {}
                -- +1 for group title and +1 for possible hyphen (v):
                local available_height_items = (available_height_px - group_label_height*2) / line_height
                for i=1,#keys do
                    table.insert(((i<available_height_items) and new_keys or overlap_leftovers), keys[i])
                end
                keys = new_keys
                table.insert(keys, {key=markup.fg(widget_instance.modifiers_color, "▽"), description=""})
            end
            if not current_column then
                current_column = {layout=wibox.layout.fixed.vertical()}
            end
            current_column.layout:add(group_label(group))

            local function insert_keys(_keys, _add_new_column)
                local max_label_width = 0
                local max_label_content = ""
                local joined_labels = ""
                for i, key in ipairs(_keys) do
                    local length = string.len(key.key or '') + string.len(key.description or '')
                    local modifiers = key.mod
                    if not modifiers or modifiers == "none" then
                        modifiers = ""
                    else
                        length = length + string.len(modifiers) + 1 -- +1 for "+" character
                        modifiers = markup.fg(widget_instance.modifiers_color, modifiers.."+")
                    end
                    local rendered_hotkey = markup.font(widget_instance.title_font,
                        modifiers .. (key.key or "") .. " "
                    ) .. markup.font(widget_instance.description_font,
                        key.description or ""
                    )
                    if length > max_label_width then
                        max_label_width = length
                        max_label_content = rendered_hotkey
                    end
                    joined_labels = joined_labels .. rendered_hotkey .. (i~=#_keys and "\n" or "")
                    end
                current_column.layout:add(wibox.widget.textbox(joined_labels))
                local max_width, _ = wibox.widget.textbox(max_label_content):get_preferred_size(s)
                max_width = max_width + widget_instance.group_margin
                if not current_column.max_width or max_width > current_column.max_width then
                    current_column.max_width = max_width
                end
                -- +1 for group label:
                current_column.height_px = (current_column.height_px or 0) +
                    awful.util.linecount(joined_labels)*line_height + group_label_height
                if _add_new_column then
                    table.insert(column_layouts, current_column)
                end
            end

            insert_keys(keys, add_new_column)
            if overlap_leftovers then
                current_column = {layout=wibox.layout.fixed.vertical()}
                insert_keys(overlap_leftovers, true)
            end
        end

        -- arrange columns into pages
        local available_width_px = width
        local pages = {}
        local columns = wibox.layout.fixed.horizontal()
        local previous_page_last_layout
        for _, item in ipairs(column_layouts) do
            if item.max_width > available_width_px then
                previous_page_last_layout:add(
                    group_label("PgDn - Next Page", beautiful.fg_normal)
                )
                table.insert(pages, columns)
                columns = wibox.layout.fixed.horizontal()
                available_width_px = width - item.max_width
                item.layout:insert(
                    1, group_label("PgUp - Prev Page", beautiful.fg_normal)
                )
            else
                available_width_px = available_width_px - item.max_width
            end
            local column_margin = wibox.container.margin()
            column_margin:set_widget(item.layout)
            column_margin:set_left(widget_instance.group_margin)
            columns:add(column_margin)
            previous_page_last_layout = item.layout
        end
        table.insert(pages, columns)

        local mywibox = wibox({
            ontop = true,
            opacity = beautiful.notification_opacity or 1,
            border_width = widget_instance.border_width,
            border_color = beautiful.fg_normal,
        })
        mywibox:geometry({
            x = wa.x + math.floor((wa.width - width - widget_instance.border_width*2) / 2),
            y = wa.y + math.floor((wa.height - height - widget_instance.border_width*2) / 2),
            width = width,
            height = height,
        })
        mywibox:set_widget(pages[1])
        mywibox:buttons(awful.util.table.join(
                awful.button({ }, 1, function () mywibox.visible=false end),
                awful.button({ }, 3, function () mywibox.visible=false end)
        ))

        local widget_obj = {}
        widget_obj.current_page = 1
        widget_obj.wibox = mywibox
        function widget_obj:page_next()
            if self.current_page == #pages then return end
            self.current_page = self.current_page + 1
            self.wibox:set_widget(pages[self.current_page])
        end
        function widget_obj:page_prev()
            if self.current_page == 1 then return end
            self.current_page = self.current_page - 1
            self.wibox:set_widget(pages[self.current_page])
        end
        function widget_obj:show()
            self.wibox.visible = true
        end
        function widget_obj:hide()
            self.wibox.visible = false
        end

        return widget_obj
    end


    --- Show popup with hotkeys help.
    -- @tparam[opt] client c Client.
    -- @tparam[opt] screen s Screen.
    function widget_instance.show_help(c, s)
        import_awful_keys()
        c = c or capi.client.focus
        s = s or (c and c.screen or awful.screen.focused())

        local available_groups = {}
        for group, _ in pairs(group_list) do
            local need_match
            for group_name, data in pairs(widget_instance.group_rules) do
                if group_name==group and (
                    data.rule or data.rule_any or data.except or data.except_any
                ) then
                    if not c or not awful.rules.matches(c, {
                        rule=data.rule,
                        rule_any=data.rule_any,
                        except=data.except,
                        except_any=data.except_any
                    }) then
                        need_match = true
                        break
                    end
                end
            end
            if not need_match then table.insert(available_groups, group) end
        end

        local joined_groups = join_plus_sort(available_groups)
        if not cached_wiboxes[s] then
            cached_wiboxes[s] = {}
        end
        if not cached_wiboxes[s][joined_groups] then
            cached_wiboxes[s][joined_groups] = create_wibox(s, available_groups)
        end
        local help_wibox = cached_wiboxes[s][joined_groups]
        help_wibox:show()

        return capi.keygrabber.run(function(_, key, event)
            if event == "release" then return end
            if key then
                if key == "Next" then
                    help_wibox:page_next()
                elseif key == "Prior" then
                    help_wibox:page_prev()
                else
                    capi.keygrabber.stop()
                    help_wibox:hide()
                end
            end
        end)
    end


    --- Add hotkey descriptions for third-party applications.
    -- @tparam table hotkeys Table with bindings,
    -- see `awful.hotkeys_popup.key.vim` as an example.
    function widget_instance.add_hotkeys(hotkeys)
        for group, bindings in pairs(hotkeys) do
            for _, binding in ipairs(bindings) do
                local modifiers = binding.modifiers
                local keys = binding.keys
                for key, description in pairs(keys) do
                    add_hotkey(key, {
                        mod=modifiers,
                        description=description,
                        group=group},
                    widget_instance.additional_hotkeys
                )
                end
            end
        end
        sort_hotkeys(widget_instance.additional_hotkeys)
    end


    return widget_instance
end

local function get_default_widget()
    if not widget.default_widget then
        widget.default_widget = widget.new()
    end
    return widget.default_widget
end

--- Show popup with hotkeys help (default widget instance will be used).
-- @tparam[opt] client c Client.
-- @tparam[opt] screen s Screen.
function widget.show_help(...)
    return get_default_widget().show_help(...)
end

--- Add hotkey descriptions for third-party applications
-- (default widget instance will be used).
-- @tparam table hotkeys Table with bindings,
-- see `awful.hotkeys_popup.key.vim` as an example.
function widget.add_hotkeys(...)
    return get_default_widget().add_hotkeys(...)
end

return widget

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
