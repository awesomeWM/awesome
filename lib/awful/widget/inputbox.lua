---------------------------------------------------------------------------
-- This widget can be used to type text and get the text from it.
--@DOC_wibox_widget_defaults_inputbox_EXAMPLE@
--
-- @author Rene Kievits
-- @copyright 2022, Rene Kievits
-- @module awful.widget.inputbox
---------------------------------------------------------------------------

local setmetatable = setmetatable
local abutton = require("awful.button")
local beautiful = require("beautiful")
local wibox = require("wibox")
local gtable = require("gears.table")
local gobject = require("gears.object")
local gstring = require("gears.string")
local gshape = require("gears.shape")
local keygrabber = require("awful.keygrabber")

local capi =
{
    selection = selection,
    mousegrabber = mousegrabber,
    mouse = mouse,
}

local inputbox = { mt = {} }
inputbox._private = {}
inputbox._private.highlight = {}

--- Formats the text with a cursor and highlights if set.
local function text_with_cursor(text, cursor_pos, self)
    local char, spacer, text_start, text_end

    local cursor_fg = beautiful.inputbox_cursor_fg or "#313131"
    local cursor_bg = beautiful.inputbox_cursor_bg or "#0dccfc"
    local text_color = beautiful.inputbox_fg or "#ffffff"
    local placeholder_text = beautiful.inputbox_placeholder_text or "Type here..."
    local placeholder_fg = beautiful.inputbox_placeholder_fg or "#777777"
    local highlight_bg = beautiful.inputbox_highlight_bg or "#35ffe4"
    local highlight_fg = beautiful.inputbox_highlight_fg or "#000000"

    if text == "" then
        return "<span foreground='" .. placeholder_fg .. "'>" .. placeholder_text .. "</span>"
    end

    if #text < cursor_pos then
        char = " "
        spacer = ""
        text_start = gstring.xml_escape(text)
        text_end = ""
    else
        local offset = 0
        if #text:sub(cursor_pos, cursor_pos) == -1 then
            offset = 1
        end
        char = gstring.xml_escape(text:sub(cursor_pos, cursor_pos + offset))
        spacer = " "
        text_start = gstring.xml_escape(text:sub(1, cursor_pos - 1))
        text_end = gstring.xml_escape(text:sub(cursor_pos + offset + 1))
    end

    if self._private.highlight and self._private.highlight.cur_pos_start and self._private.highlight.cur_pos_end then
        -- split the text into 3 parts based on the highlight and cursor position
        local text_start_highlight = gstring.xml_escape(text:sub(1, self._private.highlight.cur_pos_start - 1))
        local text_highlighted = gstring.xml_escape(text:sub(self._private.highlight.cur_pos_start,
            self._private.highlight.cur_pos_end))
        local text_end_highlight = gstring.xml_escape(text:sub(self._private.highlight.cur_pos_end + 1))

        return "<span foreground='" .. text_color .. "'>" .. text_start_highlight .. "</span>" ..
            "<span foreground='" .. highlight_fg .. "'  background='" .. highlight_bg .. "'>" ..
            text_highlighted ..
            "</span>" .. "<span foreground='" .. text_color .. "'>" .. text_end_highlight .. "</span>"
    else
        return "<span foreground='" .. text_color .. "'>" .. text_start .. "</span>" ..
            "<span background='" .. cursor_bg .. "' foreground='" .. cursor_fg .. "'>" ..
            char .. "</span>" .. "<span foreground='" .. text_color .. "'>" .. text_end .. spacer .. "</span>"
    end
end

--- The inputbox border color
--
-- @DOC_awful_widget_inputbox_border_color_EXAMPLE@
--
-- @property border_color
-- @tparam[opt=gears.color] string border_color
-- @see gears.color
-- @propemits true false
-- @propbeautiful
function inputbox:set_border_color(v)
    self._private.border_color = v
    self:emit_signal("property::border_color", v)
end

--- The inputbox border width
--
-- @DOC_awful_widget_inputbox_border_width_EXAMPLE@
--
-- @property border_width
-- @tparam[opt=0] number|nil border_width
-- @negativeallowed false
-- @propertyunit pixel
-- @propemits true false
-- @propbeautiful
function inputbox:set_border_width(v)
    self._private.border_width = v
    self:emit_signal("property::border_width", v)
end

--- The inputbox background color
--
-- @DOC_awful_widget_inputbox_bg_EXAMPLE@
--
-- @property bg
-- @tparam[opt=gears.color] string foreground
-- @see gears.color
-- @propemits true false
-- @propbeautiful
function inputbox:set_bg(v)
    self._private.bg = v
    self:emit_signal("property::bg", v)
end

--- The text foreground color
--
-- @DOC_awful_widget_inputbox_fg_EXAMPLE@
--
-- @property string
-- @tparam[opt=gears.color] string foreground
-- @see gears.color
-- @propemits true false
-- @propbeautiful
function inputbox:set_fg(v)
    self._private.fg = v
    self:emit_signal("property::fg", v)
end

--- The shape of the inputbox
--
-- @DOC_awful_widget_inputbox_shape_EXAMPLE@
--
-- @property shape
-- @tparam[opt=gears.shape.rectangle] shape|nil shape
-- @see gears.shape
-- @propemits true false
-- @propbeautiful
function inputbox:set_shape(v)
    self._private.shape = v
    self:emit_signal("property::shape", v)
end

--- Clears the current text
function inputbox:clear()
    self:set_text("")
end

function inputbox:get_text()
    return self._private.text or ""
end

function inputbox:set_text(text)
    self._private.text = text
    self:emit_signal("property::text", text)
end

--- Stop the keygrabber and mousegrabber
function inputbox:stop()
    keygrabber.stop()
    capi.mousegrabber.stop()
end

--- Init the inputbox and start the keygrabber
function inputbox:run()

    -- Init the cursor position, but causes on refocus the cursor to move to the left
    local cursor_pos = #self:get_text() + 1

    self:emit_signal("started")

    -- Init and reset(when refocused) the highlight
    self._private.highlight = {}

    -- Emitted when the keygrabber is stopped
    self:connect_signal("cancel", function()
        self:stop()
        self:emit_signal("stopped")
    end)

    -- Emitted when the keygrabber should submit the text
    self:connect_signal("submit", function(text)
        self:stop()
        self:emit_signal("stopped", text)
    end)

    keygrabber.run(function(mod, key, event)

        local mod_keys = {}
        for _, v in ipairs(mod) do
            mod_keys[v] = true
        end

        if not (event == "press") then return end

        --Escape cases
        -- Just quit and leave the text as is
        if (not mod_keys.Control) and (key == "Escape") then
            self:emit_signal("cancel")
        elseif (not mod_keys.Control and key == "KP_Enter") or (not mod_keys.Control and key == "Return") then
            self:emit_signal("submit", self:get_text())
            self:set_text("")
        end

        -- All shift, control or key cases
        if mod_keys.Shift then
            if key == "Left" then
                if cursor_pos > 1 then
                    if not self._private.highlight.cur_pos_start then
                        self._private.highlight.cur_pos_start = cursor_pos - 1
                    end
                    if not self._private.highlight.cur_pos_end then
                        self._private.highlight.cur_pos_end = cursor_pos
                    end

                    if self._private.highlight.cur_pos_start < cursor_pos then
                        self._private.highlight.cur_pos_end = self._private.highlight.cur_pos_end - 1
                    else
                        self._private.highlight.cur_pos_start = self._private.highlight.cur_pos_start - 1
                    end

                    cursor_pos = cursor_pos - 1
                end
            elseif key == "Right" then
                if #self._private.text >= cursor_pos then
                    if not self._private.highlight.cur_pos_end then
                        self._private.highlight.cur_pos_end = cursor_pos - 1
                    end
                    if not self._private.highlight.cur_pos_start then
                        self._private.highlight.cur_pos_start = cursor_pos
                    end

                    if self._private.highlight.cur_pos_end <= cursor_pos then
                        self._private.highlight.cur_pos_end = self._private.highlight.cur_pos_end + 1
                    else
                        self._private.highlight.cur_pos_start = self._private.highlight.cur_pos_start + 1
                    end
                    cursor_pos = cursor_pos + 1
                    if cursor_pos > #self._private.text + 1 then
                        self._private.highlight = {}
                    end
                end
            else
                if key:wlen() == 1 then
                    self:set_text(self._private.text:sub(1, cursor_pos - 1) ..
                        string.upper(key) .. self._private.text:sub(cursor_pos))
                    cursor_pos = cursor_pos + 1
                end
            end
        elseif mod_keys.Control then
            if key == "a" then
                -- Mark the entire text
                self._private.highlight = {
                    cur_pos_start = 1,
                    cur_pos_end = #self._private.text
                }
            elseif key == "c" then
                -- TODO: Copy the highlighted text when the selection setter gets implemented
            elseif key == "v" then
                local sel = capi.selection()
                if sel then
                    sel = sel:gsub("\n", "")
                    if self._private.highlight and self._private.highlight.cur_pos_start and
                        self._private.highlight.cur_pos_end then
                        -- insert the text into the selected part
                        local text_start = self._private.text:sub(1, self._private.highlight.cur_pos_start - 1)
                        local text_end = self._private.text:sub(self._private.highlight.cur_pos_end + 1)
                        self:set_text(text_start .. sel .. text_end)
                        self._private.highlight = {}
                        cursor_pos = #text_start + #sel + 1
                    else
                        self:set_text(self._private.text:sub(1, cursor_pos - 1) ..
                            sel .. self._private.text:sub(cursor_pos))
                        cursor_pos = cursor_pos + #sel
                    end
                end
            elseif key == "x" then
                --TODO: "cut". Copy selected then clear text, this requires to add the c function first.
                self._private.highlight = {}
            elseif key == "Left" then
                -- Find all spaces
                local spaces = {}
                local t, i = self._private.text, 0

                while t:find("%s") do
                    i = t:find("%s")
                    table.insert(spaces, i)
                    t = t:sub(1, i - 1) .. "-" .. t:sub(i + 1)
                end

                local cp = 1
                for _, v in ipairs(spaces) do
                    if (v < cursor_pos) then
                        cp = v
                    end
                end
                cursor_pos = cp
            elseif key == "Right" then
                local next_space = self._private.text:sub(cursor_pos):find("%s")
                if next_space then
                    cursor_pos = cursor_pos + next_space
                else
                    cursor_pos = #self._private.text + 1
                end
            end
        else
            if key == "BackSpace" then
                -- If text is highlighted delete that, else just delete the character to the left
                if self._private.highlight and self._private.highlight.cur_pos_start and
                    self._private.highlight.cur_pos_end then
                    local text_start = self._private.text:sub(1, self._private.highlight.cur_pos_start - 1)
                    local text_end = self._private.text:sub(self._private.highlight.cur_pos_end + 1)
                    self:set_text(text_start .. text_end)
                    self._private.highlight = {}
                    cursor_pos = #text_start + 1
                else
                    if cursor_pos > 1 then
                        self:set_text(self._private.text:sub(1, cursor_pos - 2) ..
                            self._private.text:sub(cursor_pos))
                        cursor_pos = cursor_pos - 1
                    end
                end
            elseif key == "Delete" then
                -- If text is highlighted delete that, else just delete the character to the right
                if self._private.highlight and self._private.highlight.cur_pos_start and
                    self._private.highlight.cur_pos_end then
                    local text_start = self._private.text:sub(1, self._private.highlight.cur_pos_start - 1)
                    local text_end = self._private.text:sub(self._private.highlight.cur_pos_end + 1)
                    self:set_text(text_start .. text_end)
                    self._private.highlight = {}
                    cursor_pos = #text_start + 1
                else
                    if cursor_pos <= #self._private.text then
                        self:set_text(self._private.text:sub(1, cursor_pos - 1) ..
                            self._private.text:sub(cursor_pos + 1))
                    end
                end
            elseif key == "Left" then
                -- Move cursor ro the left
                if cursor_pos > 1 then
                    cursor_pos = cursor_pos - 1
                end
                self._private.highlight = {}
            elseif key == "Right" then
                -- Move cursor to the right
                if cursor_pos <= #self._private.text then
                    cursor_pos = cursor_pos + 1
                end
                self._private.highlight = {}
            else
                -- Print every alphanumeric key
                -- It seems like gears.xmlescape doesn't support non alphanumeric characters
                if key:wlen() == 1 then
                    self:set_text(self._private.text:sub(1, cursor_pos - 1) ..
                        key .. self._private.text:sub(cursor_pos))
                    cursor_pos = cursor_pos + 1
                end
            end

            -- Make sure the cursor cannot go out of bounds
            if cursor_pos < 1 then
                cursor_pos = 1
            elseif cursor_pos > #self._private.text + 1 then
                cursor_pos = #self._private.text + 1
            end
        end

        -- Update cycle
        self.widget:update {
            text = self:get_text(),
            cursor_pos = cursor_pos,
            self = self
        }

        self:emit_signal("key_pressed", mod_keys, key)
    end)
end

local default_widget = {
    template = {
        {
            {
                {
                    widget = wibox.widget.textbox,
                    text = "",
                    halign = "left",
                    valign = "center",
                    id = "inputtext",
                },
                widget = wibox.container.margin,
                margins = 5,
            },
            widget = wibox.container.constraint,
            strategy = "exact",
            width = 400,
            height = 50,
        },
        widget = wibox.container.background,
        bg = "#212121",
        fg = "#F0F0F0",
        border_color = "#414141",
        border_width = 2,
        shape = gshape.rounded_rect,
        forced_width = 300,
        forced_height = 50,
    },
    update_callback = function(widget_template, args)
        local text = args and args.text or ""
        local cursor_pos = args and args.cursor_pos or 1
        local self = args and args.self
        self:set_text(text)

        widget_template.widget:get_children_by_id("inputtext")[1]:set_markup(text_with_cursor(text, cursor_pos, self))
        self:emit_signal("property::markup", text)
    end,
}

--- Creates a new inputbox widget
-- @tparam table args Arguments for the inputbox widget
-- @tparam string args.text The text to display in the inputbox
-- @tparam[opt=beautiful.fg_normal] string args.fg Text foreground color
-- @tparam[opt=beautiful.border_focus] string args.border_focus_color Border color when focused
-- @tparam[opt=""] string args.placeholder_text placeholder text to be shown when not focused and
-- @tparam[opt=beautiful.inputbox_placeholder_fg] string args.placeholder_fg placeholder text foreground color
-- @tparam[opt=beautiful.inputbox_cursor_bg] string args.cursor_bg Cursor background color
-- @tparam[opt=beautiful.inputbox_cursor_fg] string args.cursor_fg Cursor foreground color
-- @tparam[opt=beautiful.inputbox_highlight_bg] string args.highlight_bg Highlight background color
-- @tparam[opt=beautiful.inputbox_highlight_fg] string args.highlight_fg Highlight foreground color
-- @treturn awful.widget.inputbox The inputbox widget.
-- @constructorfct awful.widget.inputbox
function inputbox.new(args)
    args = args or {}
    assert(type(args) == "table")

    local ret = gobject { enable_properties = true }

    gtable.crush(ret, inputbox, true)

    ret.widget = args.widget_template and wibox.widget.template(args.widget_template)
        or wibox.widget.template {
            template = default_widget.template,
            update_callback = default_widget.update_callback,
            update_now = true,
        }

    gtable.crush(ret.widget, args)

    ret.widget:buttons(
        gtable.join {
            abutton({}, 1, function()
                if not keygrabber.is_running then
                    ret:run()
                end

                -- Stops the mousegrabber when not clicked on the widget
                capi.mousegrabber.run(
                    function(m)
                        if m.buttons[1] then
                            if capi.mouse.current_wibox ~= ret.widget then
                                ret:emit_signal("keygrabber::stop", "")
                                return false
                            end
                        end
                        return true
                    end,
                    -- Remove as soon as this is optional
                    capi.mouse.current_wibox.cursor
                )
            end),
            abutton({}, 3, function()
                -- TODO: Figure out how to paste with highlighted support
                -- Maybe with a signal?
            end)
        }
    )

    -- Change the cursor to "xterm" on hover over
    local old_cursor, old_wibox
    ret.widget:connect_signal(
        "mouse::enter",
        function()
            local w = capi.mouse.current_wibox
            if w then
                old_cursor, old_wibox = w.cursor, w
                w.cursor = "xterm"
            end
        end
    )

    -- Change the cursor back once leaving the widget
    ret.widget:connect_signal(
        "mouse::leave",
        function()
            old_wibox.cursor = old_cursor
            old_wibox = nil
        end
    )

    -- Initialize the text and placeholder with a first update
    ret.widget:update {
        text = args.text or "",
        cursor_pos = 1,
        self = ret
    }
    return ret.widget
end

function inputbox.mt:__call(...)
    return inputbox.new(...)
end

return setmetatable(inputbox, inputbox.mt)
