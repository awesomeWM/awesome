--------------------------------------------------------------------------------
-- @author Damien Leone &lt;damien.leone@gmail.com&gt;
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @author dodo
-- @copyright 2008, 2011 Damien Leone, Julien Danjou, dodo
-- @release @AWESOME_VERSION@
--------------------------------------------------------------------------------

local wibox = require("wibox")
local button = require("awful.button")
local util = require("awful.util")
local tags = require("awful.tag")
local keygrabber = require("awful.keygrabber")
local beautiful = require("beautiful")
local object = require("gears.object")
local surface = require("gears.surface")
local cairo = require("lgi").cairo
local setmetatable = setmetatable
local tonumber = tonumber
local string = string
local ipairs = ipairs
local pairs = pairs
local pcall = pcall
local print = print
local table = table
local type = type
local math = math
local capi = {
    timer = timer,
    screen = screen,
    mouse = mouse,
    client = client }


-- awful.menu
local menu = { mt = {} }


local table_update = function (t, set)
    for k, v in pairs(set) do
        t[k] = v
    end
    return t
end

local table_merge = function (t, set)
    for _, v in ipairs(set) do
        table.insert(t, v)
    end
end


--- Key bindings for menu navigation.
-- Keys are: up, down, exec, enter, back, close. Value are table with a list of valid
-- keys for the action, i.e. menu_keys.up =  { "j", "k" } will bind 'j' and 'k'
-- key to up action. This is common to all created menu.
-- @class table
-- @name menu_keys
menu.menu_keys = { up = { "Up", "k" },
              down = { "Down", "j" },
              back = { "Left", "h" },
              exec = { "Return" },
              enter = { "Right", "l" },
              close = { "Escape" } }


local function load_theme(a, b)
    a = a or {}
    b = b or {}
    local ret = {}
    local fallback = beautiful.get()
    if a.reset      then b = fallback end
    if a == "reset" then a = fallback end
    ret.border = a.border_color or b.menu_border_color or b.border_normal or
                 fallback.menu_border_color or fallback.border_normal
    ret.border_width= a.border_width or b.menu_border_width or b.border_width or
                      fallback.menu_border_width or fallback.border_width or 0
    ret.fg_focus = a.fg_focus or b.menu_fg_focus or b.fg_focus or
                   fallback.menu_fg_focus or fallback.fg_focus
    ret.bg_focus = a.bg_focus or b.menu_bg_focus or b.bg_focus or
                   fallback.menu_bg_focus or fallback.bg_focus
    ret.fg_normal = a.fg_normal or b.menu_fg_normal or b.fg_normal or
                    fallback.menu_fg_normal or fallback.fg_normal
    ret.bg_normal = a.bg_normal or b.menu_bg_normal or b.bg_normal or
                    fallback.menu_bg_normal or fallback.bg_normal
    ret.submenu_icon= a.submenu_icon or b.menu_submenu_icon or b.submenu_icon or
                      fallback.menu_submenu_icon or fallback.submenu_icon
    ret.submenu = a.submenu or b.menu_submenu or b.submenu or
                      fallback.menu_submenu or fallback.submenu or "▶"
    ret.height = a.height or b.menu_height or b.height or
                 fallback.menu_height or 16
    ret.width = a.width or b.menu_width or b.width or
                fallback.menu_width or 100
    ret.font = a.font or b.font or fallback.font
    for _, prop in ipairs({"width", "height", "menu_width"}) do
        if type(ret[prop]) ~= "number" then ret[prop] = tonumber(ret[prop]) end
    end
    return ret
end


local function item_position(_menu, child)
    local in_dir, other, a, b = 0, 0, "height", "width"
    local dir = _menu.layout.dir or "y"
    if dir == "x" then  a, b = b, a  end

    local in_dir, other = 0, _menu[b]
    local num = util.table.hasitem(_menu.child, child)
    if num then
        for i = 0, num - 1 do
            local item = _menu.items[i]
            if item then
                other = math.max(other, item[b])
                in_dir = in_dir + item[a]
            end
        end
    end
    local w, h = other, in_dir
    if dir == "x" then  w, h = h, w  end
    return w, h
end


local function set_coords(_menu, screen_idx, m_coords)
    local s_geometry = capi.screen[screen_idx].workarea
    local screen_w = s_geometry.x + s_geometry.width
    local screen_h = s_geometry.y + s_geometry.height

    _menu.width = _menu.wibox.width
    _menu.height = _menu.wibox.height

    _menu.x = _menu.wibox.x
    _menu.y = _menu.wibox.y

    if _menu.parent then
        local w, h = item_position(_menu.parent, _menu)
        w = w + _menu.parent.theme.border_width

        _menu.y = _menu.parent.y + h + _menu.height > screen_h and
                 screen_h - _menu.height or _menu.parent.y + h
        _menu.x = _menu.parent.x + w + _menu.width > screen_w and
                 _menu.parent.x - _menu.width or _menu.parent.x + w
    else
        if m_coords == nil then
            m_coords = capi.mouse.coords()
            m_coords.x = m_coords.x + 1
            m_coords.y = m_coords.y + 1
        end
        _menu.y = m_coords.y < s_geometry.y and s_geometry.y or m_coords.y
        _menu.x = m_coords.x < s_geometry.x and s_geometry.x or m_coords.x

        _menu.y = _menu.y + _menu.height > screen_h and
                 screen_h - _menu.height or _menu.y
        _menu.x = _menu.x + _menu.width  > screen_w and
                 screen_w - _menu.width  or _menu.x
    end

    _menu.wibox.x = _menu.x
    _menu.wibox.y = _menu.y
end


local function set_size(_menu)
    local in_dir, other, a, b = 0, 0, "height", "width"
    local dir = _menu.layout.dir or "y"
    if dir == "x" then  a, b = b, a  end
    for _, item in ipairs(_menu.items) do
        other = math.max(other, item[b])
        in_dir = in_dir + item[a]
    end
    _menu[a], _menu[b] = in_dir, other
    if in_dir > 0 and other > 0 then
        _menu.wibox[a] = in_dir
        _menu.wibox[b] = other
        return true
    end
    return false
end


local function check_access_key(_menu, key)
   for i, item in ipairs(_menu.items) do
      if item.akey == key then
            _menu:item_enter(i)
            _menu:exec(i, { exec = true })
            return
      end
   end
   if _menu.parent then
      check_access_key(_menu.parent, key)
   end
end


local function grabber(_menu, mod, key, event)
    if event ~= "press" then return end

    local sel = _menu.sel or 0
    if util.table.hasitem(menu.menu_keys.up, key) then
        local sel_new = sel-1 < 1 and #_menu.items or sel-1
        _menu:item_enter(sel_new)
    elseif util.table.hasitem(menu.menu_keys.down, key) then
        local sel_new = sel+1 > #_menu.items and 1 or sel+1
        _menu:item_enter(sel_new)
    elseif sel > 0 and util.table.hasitem(menu.menu_keys.enter, key) then
        _menu:exec(sel)
    elseif sel > 0 and util.table.hasitem(menu.menu_keys.exec, key) then
        _menu:exec(sel, { exec = true })
    elseif util.table.hasitem(menu.menu_keys.back, key) then
        _menu:hide()
    elseif util.table.hasitem(menu.menu_keys.close, key) then
        menu.get_root(_menu):hide()
    else
        check_access_key(_menu, key)
    end
end


function menu:exec(num, opts)
    opts = opts or {}
    local item = self.items[num]
    if not item then return end
    local cmd = item.cmd
    if type(cmd) == "table" then
        local action = cmd.cmd
        if #cmd == 0 then
            if opts.exec and action and type(action) == "function" then
                action()
            end
            return
        end
        if not self.child[num] then
            self.child[num] = menu.new(cmd, self)
        end
        local can_invoke_action = opts.exec and
            action and type(action) == "function" and
            (not opts.mouse or (opts.mouse and (self.auto_expand or
            (self.active_child == self.child[num] and
            self.active_child.wibox.visible))))
        if can_invoke_action then
            local visible = action(self.child[num], item)
            if not visible then
                menu.get_root(self):hide()
                return
            else
                self.child[num]:update()
            end
        end
        if self.active_child and self.active_child ~= self.child[num] then
            self.active_child:hide()
        end
        self.active_child = self.child[num]
        if not self.active_child.visible then
            self.active_child:show()
        end
    elseif type(cmd) == "string" then
        menu.get_root(self):hide()
        util.spawn(cmd)
    elseif type(cmd) == "function" then
        local visible, action = cmd(item, self)
        if not visible then
            menu.get_root(self):hide()
        else
            self:update()
            if self.items[num] then
                self:item_enter(num, opts)
            end
        end
        if action and type(action) == "function" then
            action()
        end
    end
end

function menu:item_enter(num, opts)
    opts = opts or {}
    local item = self.items[num]
    if num == nil or self.sel == num or not item then
        return
    elseif self.sel then
        self:item_leave(self.sel)
    end
    --print("sel", num, menu.sel, item.theme.bg_focus)
    item._background:set_fg(item.theme.fg_focus)
    item._background:set_bg(item.theme.bg_focus)
    self.sel = num

    if self.auto_expand and opts.hover then
        if self.active_child then
            self.active_child:hide()
            self.active_child = nil
        end

        if type(item.cmd) == "table" then
            self:exec(num, opts)
        end
    end
end


function menu:item_leave(num)
    --print("leave", num)
    local item = self.items[num]
    if item then
        item._background:set_fg(item.theme.fg_normal)
        item._background:set_bg(item.theme.bg_normal)
    end
end


--- Show a menu.
-- @param args.coords Menu position defaulting to mouse.coords()
function menu:show(args)
    args = args or {}
    local coords = args.coords or nil
    local screen_index = capi.mouse.screen

    if not set_size(self) then return end
    set_coords(self, screen_index, coords)

    keygrabber.run(self._keygrabber)
    self.wibox.visible = true
end

--- Hide a menu popup.
function menu:hide()
    -- Remove items from screen
    for i = 1, #self.items do
        self:item_leave(i)
    end
    if self.active_child then
        self.active_child:hide()
        self.active_child = nil
    end
    self.sel = nil

    keygrabber.stop(self._keygrabber)
    self.wibox.visible = false
end

--- Toggle menu visibility.
-- @param _menu The menu to show if it's hidden, or to hide if it's shown.
-- @param args.coords Menu position {x,y}
function menu:toggle(args)
    if self.wibox.visible then
        self:hide()
    else
        self:show(args)
    end
end

--- Update menu content
function menu:update()
    if self.wibox.visible then
        self:show({ coords = { x = self.x, y = self.y } })
    end
end


--- Get the elder parent so for example when you kill
-- it, it will destroy the whole family.
function menu:get_root()
    return self.parent and menu.get_root(self.parent) or self
end

--- Add a new menu entry.
-- args.new (Default: awful.menu.entry) The menu entry constructor.
-- args.theme (Optional) The menu entry theme.
-- args.* params needed for the menu entry constructor.
-- @param args The item params
-- @param index (Optional) the index where the new entry will inserted
function menu:add(args, index)
    if not args then return end
    local theme = load_theme(args.theme or {}, self.theme)
    args.theme = theme
    args.new = args.new or menu.entry
    local success, item = pcall(args.new, self, args)
    if not success then
        print("Error while creating menu entry: " .. item)
        return
    end
    if not item.widget then
        print("Error while checking menu entry: no property widget found.")
        return
    end
    item.parent = self
    item.theme = item.theme or theme
    item.width = item.width or theme.width
    item.height = item.height or theme.height
    wibox.widget.base.check_widget(item.widget)
    item._background = wibox.widget.background()
    item._background:set_widget(item.widget)
    item._background:set_fg(item.theme.fg_normal)
    item._background:set_bg(item.theme.bg_normal)


    -- Create bindings
    item._background:buttons(util.table.join(
        button({}, 3, function () self:hide() end),
        button({}, 1, function ()
            local num = util.table.hasitem(self.items, item)
            self:item_enter(num, { mouse = true })
            self:exec(num, { exec = true, mouse = true })
        end )))


    item._mouse = function ()
        local num = util.table.hasitem(self.items, item)
        self:item_enter(num, { hover = true, moue = true })
    end
    item.widget:connect_signal("mouse::enter", item._mouse)

    if index then
        self.layout:reset()
        table.insert(self.items, index, item)
        for _, i in ipairs(self.items) do
            self.layout:add(i._background)
        end
    else
        table.insert(self.items, item)
        self.layout:add(item._background)
    end
    return item
end

-- Delete menu entry at given position
-- @param _menu The menu
-- @param num The position in the table of the menu entry to be deleted; can be also the menu entry itself
function menu:delete(num)
    if type(num) == "table" then
        num = util.table.hasitem(self.items, num)
    end
    local item = self.items[num]
    if not item then return end
    item.widget:disconnect_signal("mouse::enter", item._mouse)
    item.widget.visible = false
    table.remove(self.items, num)
    if self.sel == num then
        self:item_leave(self.sel)
        self.sel = nil
    end
    self.layout:reset()
    for _, i in ipairs(self.items) do
        self.layout:add(i._background)
    end
    if self.child[num] then
         self.child[num]:hide()
        if self.active_child == self.child[num] then
            self.active_child = nil
        end
        table.remove(self.child, num)
    end
end

--------------------------------------------------------------------------------

--- Build a popup menu with running clients and shows it.
-- @param args Menu table, see new() function for more informations
-- @param item_args Table that will be merged into each item, see new() for more
--        informations.
-- @return The menu.
function menu.clients(args, item_args)
    local cls = capi.client.get()
    local cls_t = {}
    for k, c in pairs(cls) do
        cls_t[#cls_t + 1] = {
            c.name or "",
            function ()
                if not c:isvisible() then
                    tags.viewmore(c:tags(), c.screen)
                end
                capi.client.focus = c
                c:raise()
            end,
            c.icon }
        if item_args then
            if type(item_args) == "function" then
                table_merge(cls_t[#cls_t], item_args(c))
            else
                table_merge(cls_t[#cls_t], item_args)
            end
        end
    end
    args = args or {}
    args.items = args.items or {}
    table_merge(args.items, cls_t)

    local m = menu.new(args)
    m:show(args)
    return m
end

--------------------------------------------------------------------------------

--- Default awful.menu.entry constructor
-- @param parent The parent menu
-- @param args the item params
-- @return table with 'widget', 'cmd', 'akey' and all the properties the user wants to change
function menu.entry(parent, args)
    args = args or {}
    args.text = args[1] or args.text or ""
    args.cmd = args[2] or args.cmd
    args.icon = args[3] or args.icon
    local ret = {}
    -- Create the item label widget
    local label = wibox.widget.textbox()
    local key = ''
    label:set_font(args.theme.font)
    label:set_markup(string.gsub(
        util.escape(args.text), "&amp;(%w)",
        function (l)
            key = string.lower(l)
            return "<u>" .. l .. "</u>"
        end, 1))
    -- Set icon if needed
    local icon, iconbox
    local margin = wibox.layout.margin()
    margin:set_widget(label)
    if args.icon then
        icon = surface.load(args.icon)
    end
    if icon then
        local iw = icon:get_width()
        local ih = icon:get_height()
        if iw > args.theme.width or ih > args.theme.height then
            local w, h
            if ((args.theme.height / ih) * iw) > args.theme.width then
                w, h = args.theme.height, (args.theme.height / iw) * ih
            else
                w, h = (args.theme.height / ih) * iw, args.theme.height
            end
            -- We need to scale the image to size w x h
            local img = cairo.ImageSurface(cairo.Format.ARGB32, w, h)
            local cr = cairo.Context(img)
            cr:scale(w / iw, h / ih)
            cr:set_source_surface(icon, 0, 0)
            cr:paint()
            icon = img
        end
        iconbox = wibox.widget.imagebox()
        if iconbox:set_image(icon) then
            margin:set_left(2)
        else
            iconbox = nil
        end
    end
    if not iconbox then
        margin:set_left(args.theme.height + 2)
    end
    -- Create the submenu icon widget
    local submenu
    if type(args.cmd) == "table" then
        if args.theme.submenu_icon then
            submenu = wibox.widget.imagebox()
            submenu:set_image(surface.load(args.theme.submenu_icon))
        else
            submenu = wibox.widget.textbox()
            submenu:set_font(args.theme.font)
            submenu:set_text(args.theme.submenu)
        end
    end
    -- Add widgets to the wibox
    local left = wibox.layout.fixed.horizontal()
    if iconbox then
        left:add(iconbox)
    end
    -- This contains the label
    left:add(margin)

    local layout = wibox.layout.align.horizontal()
    layout:set_left(left)
    if submenu then
        layout:set_right(submenu)
    end

    return table_update(ret, {
        label = label,
        sep = submenu,
        icon = iconbox,
        widget = layout,
        cmd = args.cmd,
        akey = key,
    })
end

--------------------------------------------------------------------------------

--- Create a menu popup.
-- @param args Table containing the menu informations.<br/>
-- <ul>
-- <li> Key items: Table containing the displayed items. Each element is a table by default (when element 'new' is awful.menu.entry) containing: item name, triggered action, submenu table or function, item icon (optional). </li>
-- <li> Keys theme.[fg|bg]_[focus|normal], theme.border_color, theme.border_width, theme.submenu_icon, theme.height and theme.width override the default display for your menu and/or of your menu entry, each of them are optional. </li>
-- <li> Key auto_expand controls the submenu auto expand behaviour by setting it to true (default) or false. </li>
-- </ul>
-- @param parent Specify the parent menu if we want to open a submenu, this value should never be set by the user.
-- @usage The following function builds, and shows a menu of clients that match
-- a particular rule. Bound to a key, it can for example be used to select from
-- dozens of terminals open on several tags. With the use of
-- <code>match_any</code> instead of <code>match</code>, menu of clients with
-- different classes can also be build.
--
-- <p><code>
--                     function terminal_menu ()                           <br/>
-- &nbsp;                terms = {}                                        <br/>
-- &nbsp;                for i, c in pairs(client.get()) do                <br/>
-- &nbsp;&nbsp;            if awful.rules.match(c, {class = "URxvt"}) then <br/>
-- &nbsp;&nbsp;&nbsp;        terms[i] =                                    <br/>
-- &nbsp;&nbsp;&nbsp;          {c.name,                                    <br/>
-- &nbsp;&nbsp;&nbsp;           function()                                 <br/>
-- &nbsp;&nbsp;&nbsp;&nbsp;       awful.tag.viewonly(c:tags()[1])          <br/>
-- &nbsp;&nbsp;&nbsp;&nbsp;       client.focus = c                         <br/>
-- &nbsp;&nbsp;&nbsp;           end,                                       <br/>
-- &nbsp;&nbsp;&nbsp;           c.icon                                     <br/>
-- &nbsp;&nbsp;&nbsp;          }                                           <br/>
-- &nbsp;&nbsp;            end                                             <br/>
-- &nbsp;                end                                               <br/>
-- &nbsp;                awful.menu(terms):show()                          <br/>
--                     end                                                 <br/>
--</code></p>
function menu.new(args, parent)
    args = args or {}
    args.layout = args.layout or wibox.layout.flex.vertical
    local _menu = table_update(object(), {
        item_enter = menu.item_enter,
        item_leave = menu.item_leave,
        get_root = menu.get_root,
        delete = menu.delete,
        update = menu.update,
        toggle = menu.toggle,
        hide = menu.hide,
        show = menu.show,
        exec = menu.exec,
        add = menu.add,
        child = {},
        items = {},
        parent = parent,
        layout = args.layout(),
        theme = load_theme(args.theme or {}, parent and parent.theme) })

    if parent then
        _menu.auto_expand = parent.auto_expand
    elseif args.auto_expand ~= nil then
        _menu.auto_expand = args.auto_expand
    else
        _menu.auto_expand = true
    end

    -- Create items
    for i, v in ipairs(args) do  _menu:add(v)  end
    if args.items then
        for i, v in pairs(args.items) do  _menu:add(v)  end
    end

    _menu._keygrabber = function (...)
        grabber(_menu, ...)
    end

    _menu.wibox = wibox({
        ontop = true,
        fg = _menu.theme.fg_normal,
        bg = _menu.theme.bg_normal,
        border_color = _menu.theme.border,
        border_width = _menu.theme.border_width,
        type = "popup_menu" })
    _menu.wibox.visible = false
    _menu.wibox:set_widget(_menu.layout)
    set_size(_menu)

    _menu.x = _menu.wibox.x
    _menu.y = _menu.wibox.y
    return _menu
end

function menu.mt:__call(...)
    return menu.new(...)
end

return setmetatable(menu, menu.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
