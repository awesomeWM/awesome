---------------------------------------------------------------------------
-- A widget to write text in
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

--- Set the widget text markup
-- @property text
-- @tparam[opt=""] string text
-- @propemits true false
local function set_text_markup(self, text)
    self.widget:get_children_by_id("input_text")[1]:set_markup(text)
    self:emit_signal("property::markup", text)
end

--- Formats the text with a cursor and highlights if set.
local function text_with_cursor(text, cursor_pos, self)
    local char, spacer, text_start, text_end

    local cursor_fg = self._private.cursor_fg
    local cursor_bg = self._private.cursor_bg
    local text_color = self._private.fg
    local placeholder_text = self._private.placeholder_text
    local placeholder_fg = self._private.placeholder_fg or beautiful.fg_normal
    local highlight_bg = self._private.highlight_bg
    local highlight_fg = self._private.highlight_fg

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

--- Called when inputbox is focused to indicate a focus
function inputbox:focus()
    self.widget.border_color = self._private.border_focus_color or beautiful.border_focus
    self.widget.placeholder_text = ""
end

--- Called when the inputbox loses its focus
function inputbox:no_focus()
    self.widget.border_color = self._private.border_color or beautiful.border_normal
    if self.widget.text == "" then
        self.widget.placeholder_text = self._private.placeholder_text or ""
    end
end

--- Clears the current text
function inputbox:clear()
    self:set_text("")
end

function inputbox:get_text()
    return self._private.text
end

function inputbox:set_text(text)
    self._private.text = text
    self:emit_signal("property::text", text)
end

--- Update the text string
-- @tparam string text The text
-- @tparam[opt] number cursor_pos The cursor position
function inputbox:update(text, cursor_pos)
    self:set_text(text)
    set_text_markup(text_with_cursor(self._private.text, cursor_pos, self))
end

--- Stop the keygrabber and mousegrabber
function inputbox:stop()
    keygrabber.stop()
    capi.mousegrabber.stop()
end

--- Init the inputbox and start the keygrabber
function inputbox:run()

    -- Init the cursor position, but causes on refocus the cursor to move to the left
    local cursor_pos = #self._private.text + 1

    self:emit_signal("started")

    -- Init and reset(when refocused) the highlight
    self._private.highlight = {}

    -- Emitted when the keygrabber is stopped
    self:connect_signal("cancel", function()
        self:no_focus()
        self:stop()
        self:emit_signal("stopped")
    end)

    -- Emitted when the keygrabber should submit the text
    self:connect_signal("submit", function(text)
        self:no_focus()
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
        self:update(self._private.text, cursor_pos)

        self:emit_signal("key_pressed", mod_keys, key)
    end)
end

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

    local ret = gobject { enable_properties = true }
    gtable.crush(ret, inputbox, true)

    ret._private = {}
    ret._private.text = args.text or ""
    ret._private.fg = args.fg or beautiful.fg_normal
    ret._private.border_focus_color = args.border_focus_color or beautiful.border_focus
    ret._private.placeholder_text = args.inputbox_placeholder_text or ""
    ret._private.placeholder_fg = args.placeholder_fg or beautiful.inputbox_placeholder_fg
    ret._private.cursor_bg = args.cursor_bg or beautiful.inputbox_cursor_bg
    ret._private.cursor_fg = args.cursor_fg or beautiful.inputbox_cursor_fg
    ret._private.highlight_bg = args.highlight_bg or beautiful.inputbox_highlight_bg
    ret._private.highlight_fg = args.highlight_fg or beautiful.inputbox_highlight_fg
    ret._private.hover_cursor = args.hover_cursor or beautiful.inputbox_hover_cursor or "xterm"

    local inputbox_widget = wibox.widget {
        {
            {
                {
                    widget = wibox.widget.textbox,
                    text = ret._private.text,
                    halign = args.halign or beautiful.inputbox_halign,
                    valign = args.valign or beautiful.inputbox_valign,
                    id = "input_text",
                },
                widget = wibox.container.margin,
                margins = args.margin or beautiful.inputbox_margin,
            },
            widget = wibox.container.constraint,
            strategy = "exact",
            width = args.width or beautiful.inputbox_width,
            height = args.height or beautiful.inputbox_height,
        },
        widget = wibox.container.background,
        bg = args.bg or beautiful.bg_normal,
        fg = args.fg or beautiful.fg_normal,
        border_color = args.border_color or beautiful.border_color,
        border_width = args.border_width or beautiful.border_width,
        shape = args.shape or beautiful.shape,
    }

    ret.widget = inputbox_widget

    ret.widget:buttons(
        gtable.join {
            abutton({}, 1, function()
                if not keygrabber.is_running then
                    ret:focus()
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
    ret:update(ret._private.text, 1)

    return ret.widget
end

function inputbox.mt:__call(...)
    return inputbox.new(...)
end

return setmetatable(inputbox, inputbox.mt)
