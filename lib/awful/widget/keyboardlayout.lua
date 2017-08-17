---------------------------------------------------------------------------
-- @author Aleksey Fedotov &lt;lexa@cfotr.com&gt;
-- @copyright 2015 Aleksey Fedotov
-- @classmod awful.widget.keyboardlayout
---------------------------------------------------------------------------

local capi = {awesome = awesome}
local setmetatable = setmetatable
local textbox = require("wibox.widget.textbox")
local button = require("awful.button")
local gtable = require("gears.table")
local widget_base = require("wibox.widget.base")
local gdebug = require("gears.debug")

--- Keyboard Layout widget.
-- awful.widget.keyboardlayout
local keyboardlayout = { mt = {} }

-- As to the country-code-like symbols below, refer to the names of XKB's
-- data files in /.../xkb/symbols/*.
keyboardlayout.xkeyboard_country_code = {
    ["ad"] = true,    -- Andorra
    ["af"] = true,    -- Afganistan
    ["al"] = true,    -- Albania
    ["am"] = true,    -- Armenia
    ["ara"] = true,   -- Arabic
    ["at"] = true,    -- Austria
    ["az"] = true,    -- Azerbaijan
    ["ba"] = true,    -- Bosnia and Herzegovina
    ["bd"] = true,    -- Bangladesh
    ["be"] = true,    -- Belgium
    ["bg"] = true,    -- Bulgaria
    ["br"] = true,    -- Brazil
    ["bt"] = true,    -- Bhutan
    ["bw"] = true,    -- Botswana
    ["by"] = true,    -- Belarus
    ["ca"] = true,    -- Canada
    ["cd"] = true,    -- Congo
    ["ch"] = true,    -- Switzerland
    ["cm"] = true,    -- Cameroon
    ["cn"] = true,    -- China
    ["cz"] = true,    -- Czechia
    ["de"] = true,    -- Germany
    ["dk"] = true,    -- Denmark
    ["ee"] = true,    -- Estonia
    ["epo"] = true,   -- Esperanto
    ["es"] = true,    -- Spain
    ["et"] = true,    -- Ethiopia
    ["eu"] = true,    -- EurKey
    ["fi"] = true,    -- Finland
    ["fo"] = true,    -- Faroe Islands
    ["fr"] = true,    -- France
    ["gb"] = true,    -- United Kingdom
    ["ge"] = true,    -- Georgia
    ["gh"] = true,    -- Ghana
    ["gn"] = true,    -- Guinea
    ["gr"] = true,    -- Greece
    ["hr"] = true,    -- Croatia
    ["hu"] = true,    -- Hungary
    ["ie"] = true,    -- Ireland
    ["il"] = true,    -- Israel
    ["in"] = true,    -- India
    ["iq"] = true,    -- Iraq
    ["ir"] = true,    -- Iran
    ["is"] = true,    -- Iceland
    ["it"] = true,    -- Italy
    ["jp"] = true,    -- Japan
    ["ke"] = true,    -- Kenya
    ["kg"] = true,    -- Kyrgyzstan
    ["kh"] = true,    -- Cambodia
    ["kr"] = true,    -- Korea
    ["kz"] = true,    -- Kazakhstan
    ["la"] = true,    -- Laos
    ["latam"] = true, -- Latin America
    ["latin"] = true, -- Latin
    ["lk"] = true,    -- Sri Lanka
    ["lt"] = true,    -- Lithuania
    ["lv"] = true,    -- Latvia
    ["ma"] = true,    -- Morocco
    ["mao"] = true,   -- Maori
    ["me"] = true,    -- Montenegro
    ["mk"] = true,    -- Macedonia
    ["ml"] = true,    -- Mali
    ["mm"] = true,    -- Myanmar
    ["mn"] = true,    -- Mongolia
    ["mt"] = true,    -- Malta
    ["mv"] = true,    -- Maldives
    ["ng"] = true,    -- Nigeria
    ["nl"] = true,    -- Netherlands
    ["no"] = true,    -- Norway
    ["np"] = true,    -- Nepal
    ["ph"] = true,    -- Philippines
    ["pk"] = true,    -- Pakistan
    ["pl"] = true,    -- Poland
    ["pt"] = true,    -- Portugal
    ["ro"] = true,    -- Romania
    ["rs"] = true,    -- Serbia
    ["ru"] = true,    -- Russia
    ["se"] = true,    -- Sweden
    ["si"] = true,    -- Slovenia
    ["sk"] = true,    -- Slovakia
    ["sn"] = true,    -- Senegal
    ["sy"] = true,    -- Syria
    ["th"] = true,    -- Thailand
    ["tj"] = true,    -- Tajikistan
    ["tm"] = true,    -- Turkmenistan
    ["tr"] = true,    -- Turkey
    ["tw"] = true,    -- Taiwan
    ["tz"] = true,    -- Tanzania
    ["ua"] = true,    -- Ukraine
    ["us"] = true,    -- USA
    ["uz"] = true,    -- Uzbekistan
    ["vn"] = true,    -- Vietnam
    ["za"] = true,    -- South Africa
}

-- Callback for updating current layout.
local function update_status (self)
    self._current = awesome.xkb_get_layout_group()
    local text = ""
    if #self._layout > 0 then
        -- Please note that the group number reported by xkb_get_layout_group
        -- is lower by one than the group numbers reported by xkb_get_group_names.
        local name = self._layout[self._current+1]
        if name then
            text = " " .. name .. " "
        end
    end
    self.widget:set_text(text)
end

--- Auxiliary function for the local function update_layout().
-- Create an array whose element is a table consisting of the four fields:
-- vendor, file, section and group_idx, which all correspond to the
-- xkb_symbols pattern "vendor/file(section):group_idx".
-- @tparam string group_names The string awesome.xkb_get_group_names() returns.
-- @treturn table An array of tables whose keys are vendor, file, section, and group_idx.
function keyboardlayout.get_groups_from_group_names(group_names)
    if group_names == nil then
        return nil
    end

    -- Pattern elements to be captured.
    local word_pat = "([%w_]+)"
    local sec_pat = "(%b())"
    local idx_pat = ":(%d)"
    -- Pairs of a pattern and its callback.  In callbacks, set 'group_idx' to 1
    -- and return it if there's no specification on 'group_idx' in the given
    -- pattern.
    local pattern_and_callback_pairs = {
        -- vendor/file(section):group_idx
        ["^" .. word_pat .. "/" .. word_pat .. sec_pat .. idx_pat .. "$"]
            = function(token, pattern)
                local vendor, file, section, group_idx = string.match(token, pattern)
                return vendor, file, section, group_idx
            end,
        -- vendor/file(section)
        ["^" .. word_pat .. "/" .. word_pat .. sec_pat .. "$"]
            = function(token, pattern)
                local vendor, file, section = string.match(token, pattern)
                return vendor, file, section, 1
            end,
        -- vendor/file:group_idx
        ["^" .. word_pat .. "/" .. word_pat .. idx_pat .. "$"]
            = function(token, pattern)
                local vendor, file, group_idx = string.match(token, pattern)
                return vendor, file, nil, group_idx
            end,
        -- vendor/file
        ["^" .. word_pat .. "/" .. word_pat .. "$"]
            = function(token, pattern)
                local vendor, file = string.match(token, pattern)
                return vendor, file, nil, 1
            end,
        --  file(section):group_idx
        ["^" .. word_pat .. sec_pat .. idx_pat .. "$"]
            = function(token, pattern)
                local file, section, group_idx = string.match(token, pattern)
                return nil, file, section, group_idx
            end,
        -- file(section)
        ["^" .. word_pat .. sec_pat .. "$"]
            = function(token, pattern)
                local file, section = string.match(token, pattern)
                return nil, file, section, 1
            end,
        -- file:group_idx
        ["^" .. word_pat .. idx_pat .. "$"]
            = function(token, pattern)
                local file, group_idx = string.match(token, pattern)
                return nil, file, nil, group_idx
            end,
        -- file
        ["^" .. word_pat .. "$"]
            = function(token, pattern)
                local file = string.match(token, pattern)
                return nil, file, nil, 1
            end
    }

    -- Split 'group_names' into 'tokens'.  The separator is "+".
    local tokens = {}
    string.gsub(group_names, "[^+]+", function(match)
        table.insert(tokens, match)
    end)

    -- For each token in 'tokens', check if it matches one of the patterns in
    -- the array 'pattern_and_callback_pairs', where the patterns are used as
    -- key.  If a match is found, extract captured strings using the
    -- corresponding callback function.  Check if those extracted is country
    -- specific part of a layout.  If so, add it to 'layout_groups'; otherwise,
    -- ignore it.
    local layout_groups = {}
    for i = 1, #tokens do
        for pattern, callback in pairs(pattern_and_callback_pairs) do
            local vendor, file, section, group_idx = callback(tokens[i], pattern)
            if file then
                if not keyboardlayout.xkeyboard_country_code[file] then
                    break
                end

                if section then
                    section = string.gsub(section, "%(([%w-_]+)%)", "%1")
                end

                table.insert(layout_groups, { vendor = vendor,
                                              file = file,
                                              section = section,
                                              group_idx = tonumber(group_idx) })
                break
            end
        end
    end

    return layout_groups
end

-- Callback for updating list of layouts
local function update_layout(self)
    self._layout = {};
    local layouts = keyboardlayout.get_groups_from_group_names(awesome.xkb_get_group_names())
    if layouts == nil or layouts[1] == nil then
        gdebug.print_error("Failed to get list of keyboard groups")
        return
    end
    if #layouts == 1 then
        layouts[1].group_idx = 1
    end
    for _, v in ipairs(layouts) do
        self._layout[v.group_idx] = self.layout_name(v)
    end
    update_status(self)
end

--- Create a keyboard layout widget. It shows current keyboard layout name in a textbox.
-- @return A keyboard layout widget.
function keyboardlayout.new()
    local widget = textbox()
    local self = widget_base.make_widget(widget)

    self.widget = widget

    self.layout_name = function(v)
        local name = v.file
        if v.section ~= nil then
            name = name .. "(" .. v.section .. ")"
        end
        return name
    end

    self.next_layout = function()
        self.set_layout((self._current + 1) % (#self._layout + 1))
    end

    self.set_layout = function(group_number)
        if (0 > group_number) or (group_number > #self._layout) then
            error("Invalid group number: " .. group_number ..
                    "expected number from 0 to " .. #self._layout)
            return;
        end
        awesome.xkb_set_layout_group(group_number);
    end

    update_layout(self);

    -- callback for processing layout changes
    capi.awesome.connect_signal("xkb::map_changed",
                                function () update_layout(self) end)
    capi.awesome.connect_signal("xkb::group_changed",
                                function () update_status(self) end);

    -- Mouse bindings
    self:buttons(
        gtable.join(button({ }, 1, self.next_layout))
    )

    return self
end

local _instance = nil;

function keyboardlayout.mt:__call(...)
    if _instance == nil then
        _instance = keyboardlayout.new(...)
    end
    return _instance
end

return setmetatable(keyboardlayout, keyboardlayout.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
