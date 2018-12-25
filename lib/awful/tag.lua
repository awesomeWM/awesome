---------------------------------------------------------------------------
--- Useful functions for tag manipulation.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module tag
---------------------------------------------------------------------------

-- Grab environment we need
local gdebug = require("gears.debug")
local ascreen = require("awful.screen")
local beautiful = require("beautiful")
local gmath = require("gears.math")
local object = require("gears.object")
local timer = require("gears.timer")
local gtable = require("gears.table")
local alayout = nil
local pairs = pairs
local ipairs = ipairs
local table = table
local setmetatable = setmetatable
local capi =
{
    tag = tag,
    screen = screen,
    mouse = mouse,
    client = client,
    root = root
}

local function get_screen(s)
    return s and capi.screen[s]
end

local tag = {object = {},  mt = {} }

-- Private data
local data = {}
data.history = {}

-- History functions
tag.history = {}
tag.history.limit = 20

-- Default values
local defaults = {}

-- The gap between clients (in points).
defaults.gap                 = 0

-- The default gap_count.
defaults.gap_single_client   = true

-- The default master fill policy.
defaults.master_fill_policy  = "expand"

-- The default master width factor.
defaults.master_width_factor = 0.5

-- The default master count.
defaults.master_count        = 1

-- The default column count.
defaults.column_count        = 1

-- screen.tags depend on index, it cannot be used by awful.tag
local function raw_tags(scr)
    local tmp_tags = {}
    for _, t in ipairs(root.tags()) do
        if get_screen(t.screen) == scr then
            table.insert(tmp_tags, t)
        end
    end

    return tmp_tags
end

local function custom_layouts(self)
    local cls = tag.getproperty(self, "_custom_layouts")

    if not cls then
        cls = {}
        tag.setproperty(self, "_custom_layouts", cls)
    end

    return cls
end

-- Update the "user visible" list of layouts. If `from` and `to` are not the
-- same, then `from` will be replaced. This is necessary for either the layouts
-- defined as a function (called "template" below) and object oriented, stateful
-- layouts where the original entry is only a constructor.
local function update_layouts(self, from, to)
    if not to then return end

    alayout = alayout or require("awful.layout")
    local override = tag.getproperty(self, "_layouts")

    local pos = from and gtable.hasitem(override or {}, from) or nil

    -- There is an override and the layout template is part of it, replace by
    -- the instance.
    if override and pos and from ~= to then
        assert(type(pos) == 'number')
        override[pos] = to
        self:emit_signal("property::layouts")
        return
    end

    -- Only add to the custom_layouts and preserve the ability to globally
    -- set the layouts.
    if override and not pos then
        table.insert(override, to)
        self:emit_signal("property::layouts")
        return
    end

    pos = from and gtable.hasitem(alayout.layouts, from) or nil

    local cls = custom_layouts(self)

    -- The new layout is part of the global layouts. Fork the list.
    if pos and from ~= to then
        local cloned = gtable.clone(alayout.layouts, false)
        cloned[pos] = to
        gtable.merge(cloned, cls)
        self.layouts = cloned
        return
    end

    if pos then return end

    if gtable.hasitem(cls, to) then return end

    -- This layout is unknown, add it to the custom list
    table.insert(cls, to)
    self:emit_signal("property::layouts")
end

--- The number of elements kept in the history.
-- @tfield integer awful.tag.history.limit
-- @tparam[opt=20] integer limit

--- The tag index.
--
-- The index is the position as shown in the `awful.widget.taglist`.
--
-- **Signal:**
--
-- * *property::index*
--
-- @property index
-- @param integer
-- @treturn number The tag index.

function tag.object.set_index(self, idx)
    local scr = get_screen(tag.getproperty(self, "screen"))

    -- screen.tags cannot be used as it depend on index
    local tmp_tags = raw_tags(scr)

    -- sort the tags by index
    table.sort(tmp_tags, function(a, b)
        local ia, ib = tag.getproperty(a, "index"), tag.getproperty(b, "index")
        return (ia or math.huge) < (ib or math.huge)
    end)

    if (not idx) or (idx < 1) or (idx > #tmp_tags) then
        return
    end

    local rm_index = nil

    for i, t in ipairs(tmp_tags) do
        if t == self then
            table.remove(tmp_tags, i)
            rm_index = i
            break
        end
    end

    table.insert(tmp_tags, idx, self)
    for i = idx < rm_index and idx or rm_index, #tmp_tags do
        local tmp_tag = tmp_tags[i]
        tag.object.set_screen(tmp_tag, scr)
        tag.setproperty(tmp_tag, "index", i)
    end
end

function tag.object.get_index(query_tag)

    local idx = tag.getproperty(query_tag, "index")

    if idx then return idx end

    -- Get an unordered list of tags
    local tags = raw_tags(query_tag.screen)

    -- Too bad, lets compute it
    for i, t in ipairs(tags) do
        if t == query_tag then
            tag.setproperty(t, "index", i)
            return i
        end
    end
end

--- Move a tag to an absolute position in the screen[]:tags() table.
-- @deprecated awful.tag.move
-- @param new_index Integer absolute position in the table to insert.
-- @param target_tag The tag that should be moved. If null, the currently
-- selected tag is used.
-- @see index
function tag.move(new_index, target_tag)
    gdebug.deprecate("Use t.index = new_index instead of awful.tag.move", {deprecated_in=4})

    target_tag = target_tag or ascreen.focused().selected_tag
    tag.object.set_index(target_tag, new_index)
end

--- Swap 2 tags
-- @function tag.swap
-- @param tag2 The second tag
-- @see client.swap
function tag.object.swap(self, tag2)
    local idx1, idx2 = tag.object.get_index(self), tag.object.get_index(tag2)
    local scr2, scr1 = tag.getproperty(tag2, "screen"), tag.getproperty(self, "screen")

    -- If they are on the same screen, avoid recomputing the whole table
    -- for nothing.
    if scr1 == scr2 then
        tag.setproperty(self, "index", idx2)
        tag.setproperty(tag2, "index", idx1)
    else
        tag.object.set_screen(tag2, scr1)
        tag.object.set_index (tag2, idx1)
        tag.object.set_screen(self, scr2)
        tag.object.set_index (self, idx2)
    end
end

--- Swap 2 tags
-- @deprecated awful.tag.swap
-- @see tag.swap
-- @param tag1 The first tag
-- @param tag2 The second tag
function tag.swap(tag1, tag2)
    gdebug.deprecate("Use t:swap(tag2) instead of awful.tag.swap", {deprecated_in=4})

    tag.object.swap(tag1, tag2)
end

--- Add a tag.
--
-- This function allow to create tags from a set of properties:
--
--    local t = awful.tag.add("my new tag", {
--        screen = screen.primary,
--        layout = awful.layout.suit.max,
--    })
--
-- @function awful.tag.add
-- @param name The tag name, a string
-- @param props The tags inital properties, a table
-- @return The created tag
-- @see tag.delete
function tag.add(name, props)
    local properties = props or {}

    -- Be sure to set the screen before the tag is activated to avoid function
    -- connected to property::activated to be called without a valid tag.
    -- set properties cannot be used as this has to be set before the first
    -- signal is sent
    properties.screen = get_screen(properties.screen or ascreen.focused())
    -- Index is also required
    properties.index = properties.index or #raw_tags(properties.screen)+1

    local newtag = capi.tag{ name = name }

    -- Start with a fresh property table to avoid collisions with unsupported data
    newtag.data.awful_tag_properties = {screen=properties.screen, index=properties.index}

    newtag.activated = true

    for k, v in pairs(properties) do
        -- `rawget` doesn't work on userdata, `:clients()` is the only relevant
        -- entry.
        if k == "clients" or tag.object[k] then
            newtag[k](newtag, v)
        else
            newtag[k] = v
        end
    end

    return newtag
end

--- Create a set of tags and attach it to a screen.
-- @function awful.tag.new
-- @param names The tag name, in a table
-- @param screen The tag screen, or 1 if not set.
-- @param layout The layout or layout table to set for this tags by default.
-- @return A table with all created tags.
function tag.new(names, screen, layout)
    screen = get_screen(screen or 1)
    -- True if `layout` should be used as the layout of each created tag
    local have_single_layout = (not layout) or (layout.arrange and layout.name)
    local tags = {}
    for id, name in ipairs(names) do
        local l = layout
        if not have_single_layout then
            l = layout[id] or layout[1]
        end
        table.insert(tags, id, tag.add(name, {screen = screen, layout = l}))
        -- Select the first tag.
        if id == 1 then
            tags[id].selected = true
        end
    end

    return tags
end

--- Find a suitable fallback tag.
-- @function awful.tag.find_fallback
-- @param screen The screen to look for a tag on. [awful.screen.focused()]
-- @param invalids A table of tags we consider unacceptable. [selectedlist(scr)]
function tag.find_fallback(screen, invalids)
    local scr = screen or ascreen.focused()
    local t = invalids or scr.selected_tags

    for _, v in pairs(scr.tags) do
        if not gtable.hasitem(t, v) then return v end
    end
end

--- Delete a tag.
--
-- To delete the current tag:
--
--    mouse.screen.selected_tag:delete()
--
-- @function tag.delete
-- @see awful.tag.add
-- @see awful.tag.find_fallback
-- @tparam[opt=awful.tag.find_fallback()] tag fallback_tag Tag to assign
--  stickied tags to.
-- @tparam[opt=false] boolean force Move even non-sticky clients to the fallback
-- tag.
-- @return Returns true if the tag is successfully deleted.
-- If there are no clients exclusively on this tag then delete it. Any
-- stickied clients are assigned to the optional 'fallback_tag'.
-- If after deleting the tag there is no selected tag, try and restore from
-- history or select the first tag on the screen.
function tag.object.delete(self, fallback_tag, force)
    -- abort if the taf isn't currently activated
    if not self.activated then return false end

    local target_scr = get_screen(tag.getproperty(self, "screen"))
    local tags       = target_scr.tags
    local idx        = tag.object.get_index(self)
    local ntags      = #tags

    -- We can't use the target tag as a fallback.
    if fallback_tag == self then return false end

    -- No fallback_tag provided, try and get one.
    if fallback_tag == nil then
        fallback_tag = tag.find_fallback(target_scr, {self})
    end

    -- Abort if we would have un-tagged clients.
    local clients = self:clients()
    if #clients > 0 and fallback_tag == nil then return false end

    -- Move the clients we can off of this tag.
    for _, c in pairs(clients) do
        local nb_tags = #c:tags()

        -- If a client has only this tag, or stickied clients with
        -- nowhere to go, abort.
        if (not c.sticky and nb_tags == 1 and not force) then
            return
        -- If a client has multiple tags, then do not move it to fallback
        elseif nb_tags < 2 then
            c:tags({fallback_tag})
        end
    end

    -- delete the tag
    self.data.awful_tag_properties.screen = nil
    self.activated = false

    -- Update all indexes
    for i=idx+1, #tags do
        tag.setproperty(tags[i], "index", i-1)
    end

    -- If no tags are visible (and we did not delete the lasttag), try and
    -- view one. The > 1 is because ntags is no longer synchronized with the
    -- current count.
    if target_scr.selected_tag == nil and ntags > 1 then
        tag.history.restore(target_scr, 1)
        if target_scr.selected_tag == nil then
            local other_tag = tags[tags[1] == self and 2 or 1]
            if other_tag then
                other_tag.selected = true
            end
        end
    end

    return true
end

--- Delete a tag.
-- @deprecated awful.tag.delete
-- @see tag.delete
-- @param target_tag Optional tag object to delete. [selected()]
-- @param fallback_tag Tag to assign stickied tags to. [~selected()]
-- @return Returns true if the tag is successfully deleted, nil otherwise.
-- If there are no clients exclusively on this tag then delete it. Any
-- stickied clients are assigned to the optional 'fallback_tag'.
-- If after deleting the tag there is no selected tag, try and restore from
-- history or select the first tag on the screen.
function tag.delete(target_tag, fallback_tag)
    gdebug.deprecate("Use t:delete(fallback_tag) instead of awful.tag.delete", {deprecated_in=4})

    return tag.object.delete(target_tag, fallback_tag)
end

--- Update the tag history.
-- @function awful.tag.history.update
-- @param obj Screen object.
function tag.history.update(obj)
    local s = get_screen(obj)
    local curtags = s.selected_tags
    -- create history table
    if not data.history[s] then
        data.history[s] = {}
    else
        if data.history[s].current then
            -- Check that the list is not identical
            local identical = #data.history[s].current == #curtags
            if identical then
                for idx, _tag in ipairs(data.history[s].current) do
                    if curtags[idx] ~= _tag then
                        identical = false
                        break
                    end
                end
            end

            -- Do not update history the table are identical
            if identical then return end
        end

        -- Limit history
        if #data.history[s] >= tag.history.limit then
            for i = tag.history.limit, #data.history[s] do
                data.history[s][i] = nil
            end
        end
    end

    -- store previously selected tags in the history table
    table.insert(data.history[s], 1, data.history[s].current)
    data.history[s].previous = data.history[s][1]
    -- store currently selected tags
    data.history[s].current = setmetatable(curtags, { __mode = 'v' })
end

--- Revert tag history.
-- @function awful.tag.history.restore
-- @param screen The screen.
-- @param idx Index in history. Defaults to "previous" which is a special index
-- toggling between last two selected sets of tags. Number (eg 1) will go back
-- to the given index in history.
function tag.history.restore(screen, idx)
    local s = get_screen(screen or ascreen.focused())
    local i = idx or "previous"
    local sel = s.selected_tags
    -- do nothing if history empty
    if not data.history[s] or not data.history[s][i] then return end
    -- if all tags been deleted, try next entry
    if #data.history[s][i] == 0 then
        if i == "previous" then i = 0 end
        tag.history.restore(s, i + 1)
        return
    end
    -- deselect all tags
    tag.viewnone(s)
    -- select tags from the history entry
    for _, t in ipairs(data.history[s][i]) do
        if t.activated and t.screen then
            t.selected = true
        end
    end
    -- update currently selected tags table
    data.history[s].current = data.history[s][i]
    -- store previously selected tags
    data.history[s].previous = setmetatable(sel, { __mode = 'v' })
    -- remove the reverted history entry
    if i ~= "previous" then table.remove(data.history[s], i) end

    s:emit_signal("tag::history::update")
end

--- Get a list of all tags on a screen
-- @deprecated awful.tag.gettags
-- @tparam screen s Screen
-- @return A table with all available tags
-- @see screen.tags
function tag.gettags(s)
    gdebug.deprecate("Use s.tags instead of awful.tag.gettags", {deprecated_in=4})

    s = get_screen(s)

    return s and s.tags or {}
end

--- Find a tag by name.
-- @tparam screen s The screen of the tag
-- @tparam string name The name of the tag
-- @return The tag found, or `nil`
-- @usage -- For the current screen
-- local t = awful.tag.find_by_name(awful.screen.focused(), "name")
--
-- -- For a screen index
-- local t = awful.tag.find_by_name(screen[1], "name")
--
-- -- For all screens
-- local t = awful.tag.find_by_name(nil, "name")
function tag.find_by_name(s, name)
    --TODO v5: swap the arguments and make screen [opt]
    local tags = s and s.tags or root.tags()
    for _, t in ipairs(tags) do
        if name == t.name then
            return t
        end
    end
end

--- The tag screen.
--
-- **Signal:**
--
-- * *property::screen*
--
-- @property screen
-- @param screen
-- @see screen

function tag.object.set_screen(t, s)

    s = get_screen(s or ascreen.focused())
    local sel = t.selected
    local old_screen = get_screen(tag.getproperty(t, "screen"))

    if s == old_screen then return end

    -- Keeping the old index make very little sense when changing screen
    tag.setproperty(t, "index", nil)

    -- Change the screen
    tag.setproperty(t, "screen", s)
    if s then
        tag.setproperty(t, "index", #s:get_tags(true))
    end

    -- Make sure the client's screen matches its tags
    for _,c in ipairs(t:clients()) do
        c.screen = s --Move all clients
        c:tags({t})
    end

    if old_screen then
        -- Update all indexes
        for i,t2 in ipairs(old_screen.tags) do
            tag.setproperty(t2, "index", i)
        end

        -- Restore the old screen history if the tag was selected
        if sel then
            tag.history.restore(old_screen, 1)
        end
    end
end

--- Set a tag's screen
-- @deprecated awful.tag.setscreen
-- @see screen
-- @param s Screen
-- @param t tag object
function tag.setscreen(s, t)
    -- For API consistency, the arguments have been swapped for Awesome 3.6
    -- this method is already deprecated, so be silent and swap the args
    if type(t) == "number" then
        s, t = t, s
    end

    gdebug.deprecate("Use t.screen = s instead of awful.tag.setscreen(t, s)", {deprecated_in=4})

    tag.object.set_screen(t, s)
end

--- Get a tag's screen
-- @deprecated awful.tag.getscreen
-- @see screen
-- @param[opt] t tag object
-- @return Screen number
function tag.getscreen(t)
    gdebug.deprecate("Use t.screen instead of awful.tag.getscreen(t)", {deprecated_in=4})

    -- A new getter is not required

    t = t or ascreen.focused().selected_tag
    local prop = tag.getproperty(t, "screen")
    return prop and prop.index
end

--- Return a table with all visible tags
-- @deprecated awful.tag.selectedlist
-- @param s Screen.
-- @return A table with all selected tags.
-- @see screen.selected_tags
function tag.selectedlist(s)
    gdebug.deprecate("Use s.selected_tags instead of awful.tag.selectedlist", {deprecated_in=4})

    s = get_screen(s or ascreen.focused())

    return s.selected_tags
end

--- Return only the first visible tag.
-- @deprecated awful.tag.selected
-- @param s Screen.
-- @see screen.selected_tag
function tag.selected(s)
    gdebug.deprecate("Use s.selected_tag instead of awful.tag.selected", {deprecated_in=4})

    s = get_screen(s or ascreen.focused())

    return s.selected_tag
end

--- The default master width factor
--
-- @beautiful beautiful.master_width_factor
-- @param number (default: 0.5)
-- @see master_width_factor
-- @see gap

--- The tag master width factor.
--
-- The master width factor is one of the 5 main properties used to configure
-- the `layout`. Each layout interpret (or ignore) this property differenly.
--
-- See the layout suit documentation for information about how the master width
-- factor is used.
--
-- **Signal:**
--
-- * *property::mwfact* (deprecated)
-- * *property::master_width_factor*
--
-- @property master_width_factor
-- @param number Between 0 and 1
-- @see master_count
-- @see column_count
-- @see master_fill_policy
-- @see gap

function tag.object.set_master_width_factor(t, mwfact)
    if mwfact >= 0 and mwfact <= 1 then
        tag.setproperty(t, "mwfact", mwfact)
        tag.setproperty(t, "master_width_factor", mwfact)
    end
end

function tag.object.get_master_width_factor(t)
    return tag.getproperty(t, "master_width_factor")
        or beautiful.master_width_factor
        or defaults.master_width_factor
end

--- Set master width factor.
-- @deprecated awful.tag.setmwfact
-- @see master_fill_policy
-- @see master_width_factor
-- @param mwfact Master width factor.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.setmwfact(mwfact, t)
    gdebug.deprecate("Use t.master_width_factor = mwfact instead of awful.tag.setmwfact", {deprecated_in=4})

    tag.object.set_master_width_factor(t or ascreen.focused().selected_tag, mwfact)
end

--- Increase master width factor.
-- @function awful.tag.incmwfact
-- @see master_width_factor
-- @param add Value to add to master width factor.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.incmwfact(add, t)
    t = t or t or ascreen.focused().selected_tag
    tag.object.set_master_width_factor(t, tag.object.get_master_width_factor(t) + add)
end

--- Get master width factor.
-- @deprecated awful.tag.getmwfact
-- @see master_width_factor
-- @see master_fill_policy
-- @param[opt] t The tag.
function tag.getmwfact(t)
    gdebug.deprecate("Use t.master_width_factor instead of awful.tag.getmwfact", {deprecated_in=4})

    return tag.object.get_master_width_factor(t or ascreen.focused().selected_tag)
end

--- An ordered list of layouts.
-- `awful.tag.layout` Is usually defined in `rc.lua`. It store the list of
-- layouts used when selecting the previous and next layouts. This is the
-- default:
--
--     -- Table of layouts to cover with awful.layout.inc, order matters.
--     awful.layout.layouts = {
--         awful.layout.suit.floating,
--         awful.layout.suit.tile,
--         awful.layout.suit.tile.left,
--         awful.layout.suit.tile.bottom,
--         awful.layout.suit.tile.top,
--         awful.layout.suit.fair,
--         awful.layout.suit.fair.horizontal,
--         awful.layout.suit.spiral,
--         awful.layout.suit.spiral.dwindle,
--         awful.layout.suit.max,
--         awful.layout.suit.max.fullscreen,
--         awful.layout.suit.magnifier,
--         awful.layout.suit.corner.nw,
--         -- awful.layout.suit.corner.ne,
--         -- awful.layout.suit.corner.sw,
--         -- awful.layout.suit.corner.se,
--     }
--
-- @field awful.tag.layouts

--- The tag client layout.
--
-- This property hold the layout. A layout can be either stateless or stateful.
-- Stateless layouts are used by default by Awesome. They tile clients without
-- any other overhead. They take an ordered list of clients and place them on
-- the screen. Stateful layouts create an object instance for each tags and
-- can store variables and metadata. Because of this, they are able to change
-- over time and be serialized (saved).
--
-- Both types of layouts have valid usage scenarios.
--
-- **Stateless layouts:**
--
-- These layouts are stored in `awful.layout.suit`. They expose a table with 2
-- fields:
--
-- * **name** (*string*): The layout name. This should be unique.
-- * **arrange** (*function*): The function called when the clients need to be
--     placed. The only parameter is a table or arguments returned by
--     `awful.layout.parameters`
--
-- **Stateful layouts:**
--
-- The stateful layouts API is the same as stateless, but they are a function
-- returining a layout instead of a layout itself. They also should have an
-- `is_dynamic = true` property. If they don't, `awful.tag` will create a new
-- instance everytime the layout is set. If they do, the instance will be
-- cached and re-used.
--
-- **Signal:**
--
-- * *property::layout*
--
-- @property layout
-- @see awful.tag.layouts
-- @tparam layout|function layout A layout table or a constructor function
-- @return The layout

--- The (proposed) list of available layouts for this tag.
--
-- This property allows to define a subset (or superset) of layouts available
-- in the "rotation table". In the default configuration file, `Mod4+Space`
-- and `Mod4+Shift+Space` are used to switch between tags. The
-- `awful.widget.layoutlist` also uses this as its default layout filter.
--
-- By default, it will be the same as `awful.layout.layouts` unless there the
-- a layout not present is used. If that's the case they will be added at the
-- front of the list.
--
-- @property layouts
-- @param table
-- @see awful.layout.layouts
-- @see layout

function tag.object.set_layout(t, layout)

    local template = nil

    -- Check if the signature match a stateful layout
    if type(layout) == "function" or (
        type(layout) == "table"
        and getmetatable(layout)
        and getmetatable(layout).__call
    ) then
        if not t.dynamic_layout_cache then
            t.dynamic_layout_cache = {}
        end

        local instance = t.dynamic_layout_cache[layout] or layout(t)

        -- Always make sure the layout is notified it is enabled
        if tag.getproperty(t, "screen").selected_tag == t and instance.wake_up then
            instance:wake_up()
        end

        -- Avoid creating the same layout twice, use layout:reset() to reset
        if instance.is_dynamic then
            t.dynamic_layout_cache[layout] = instance
        end

        template = layout
        layout = instance
    end

    tag.setproperty(t, "layout", layout)

    update_layouts(t, template or layout, layout)

    return layout
end

function tag.object.get_layouts(self)
    local override = tag.getproperty(self, "_layouts")

    if override then
        return override
    end

    -- Required to get the default/fallback list of layouts
    alayout = alayout or require("awful.layout")

    local cls = custom_layouts(self)

    -- Without the clone, the custom_layouts would grow
    return #cls > 0 and gtable.merge(gtable.clone(cls, false), alayout.layouts) or
        alayout.layouts
end

function tag.object.set_layouts(self, layouts)
    tag.setproperty(self, "_custom_layouts", {})
    tag.setproperty(self, "_layouts", gtable.clone(layouts, false))

    local cur = tag.getproperty(self, "layout")
    update_layouts(self, cur, cur)

    self:emit_signal("property::layouts")
end

function tag.object.get_layout(t)
    local l = tag.getproperty(t, "layout")
    if l then return l end

    local layouts = tag.getproperty(t, "_layouts")

    return layouts and layouts[1]
        or require("awful.layout.suit.floating")
end

--- Set layout.
-- @deprecated awful.tag.setlayout
-- @see layout
-- @param layout a layout table or a constructor function
-- @param t The tag to modify
-- @return The layout
function tag.setlayout(layout, t)
    gdebug.deprecate("Use t.layout = layout instead of awful.tag.setlayout", {deprecated_in=4})

    return tag.object.set_layout(t, layout)
end

--- Define if the tag must be deleted when the last client is untagged.
--
-- This is useful to create "throw-away" tags for operation like 50/50
-- side-by-side views.
--
--    local t = awful.tag.add("Temporary", {
--         screen   = client.focus.screen,
--         volatile = true,
--         clients  = {
--             client.focus,
--             awful.client.focus.history.get(client.focus.screen, 1)
--         }
--    }
--
-- **Signal:**
--
-- * *property::volatile*
--
-- @property volatile
-- @param boolean

-- Volatile accessors are implicit

--- Set if the tag must be deleted when the last client is untagged
-- @deprecated awful.tag.setvolatile
-- @see volatile
-- @tparam boolean volatile If the tag must be deleted when the last client is untagged
-- @param t The tag to modify, if null tag.selected() is used.
function tag.setvolatile(volatile, t)
    gdebug.deprecate("Use t.volatile = volatile instead of awful.tag.setvolatile", {deprecated_in=4})

    tag.setproperty(t, "volatile", volatile)
end

--- Get if the tag must be deleted when the last client closes
-- @deprecated awful.tag.getvolatile
-- @see volatile
-- @param t The tag to modify, if null tag.selected() is used.
-- @treturn boolean If the tag will be deleted when the last client is untagged
function tag.getvolatile(t)
    gdebug.deprecate("Use t.volatile instead of awful.tag.getvolatile", {deprecated_in=4})

    return tag.getproperty(t, "volatile") or false
end

--- The default gap.
--
-- @beautiful beautiful.useless_gap
-- @param number (default: 0)
-- @see gap
-- @see gap_single_client

--- The gap (spacing, also called `useless_gap`) between clients.
--
-- This property allow to waste space on the screen in the name of style,
-- unicorns and readability.
--
-- **Signal:**
--
-- * *property::useless_gap*
--
-- @property gap
-- @param number The value has to be greater than zero.
-- @see gap_single_client

function tag.object.set_gap(t, useless_gap)
    if useless_gap >= 0 then
        tag.setproperty(t, "useless_gap", useless_gap)
    end
end

function tag.object.get_gap(t)
    return tag.getproperty(t, "useless_gap")
        or beautiful.useless_gap
        or defaults.gap
end

--- Set the spacing between clients
-- @deprecated awful.tag.setgap
-- @see gap
-- @param useless_gap The spacing between clients
-- @param t The tag to modify, if null tag.selected() is used.
function tag.setgap(useless_gap, t)
    gdebug.deprecate("Use t.gap = useless_gap instead of awful.tag.setgap", {deprecated_in=4})

    tag.object.set_gap(t or ascreen.focused().selected_tag, useless_gap)
end

--- Increase the spacing between clients
-- @function awful.tag.incgap
-- @see gap
-- @param add Value to add to the spacing between clients
-- @param t The tag to modify, if null tag.selected() is used.
function tag.incgap(add, t)
    t = t or t or ascreen.focused().selected_tag
    tag.object.set_gap(t, tag.object.get_gap(t) + add)
end

--- Enable gaps for a single client.
--
-- @beautiful beautiful.gap_single_client
-- @param boolean (default: true)
-- @see gap
-- @see gap_single_client

--- Enable gaps for a single client.
--
-- **Signal:**
--
-- * *property::gap\_single\_client*
--
-- @property gap_single_client
-- @param boolean Enable gaps for a single client

function tag.object.set_gap_single_client(t, gap_single_client)
    tag.setproperty(t, "gap_single_client", gap_single_client == true)
end

function tag.object.get_gap_single_client(t)
    local val = tag.getproperty(t, "gap_single_client")
    if val ~= nil then
        return val
    end
    val = beautiful.gap_single_client
    if val ~= nil then
        return val
    end
    return defaults.gap_single_client
end

--- Get the spacing between clients.
-- @deprecated awful.tag.getgap
-- @see gap
-- @tparam[opt=tag.selected()] tag t The tag.
-- @tparam[opt] int numclients Number of (tiled) clients.  Passing this will
--   return 0 for a single client.  You can override this function to change
--   this behavior.
function tag.getgap(t, numclients)
    gdebug.deprecate("Use t.gap instead of awful.tag.getgap", {deprecated_in=4})

    if numclients == 1 then
        return 0
    end

    return tag.object.get_gap(t or ascreen.focused().selected_tag)
end

--- The default fill policy.
--
-- ** Possible values**:
--
-- * *expand*: Take all the space
-- * *master_width_factor*: Only take the ratio defined by the
--   `master_width_factor`
--
-- @beautiful beautiful.master_fill_policy
-- @param string (default: "expand")
-- @see master_fill_policy

--- Set size fill policy for the master client(s).
--
-- ** Possible values**:
--
-- * *expand*: Take all the space
-- * *master_width_factor*: Only take the ratio defined by the
--   `master_width_factor`
--
-- **Signal:**
--
-- * *property::master_fill_policy*
--
-- @property master_fill_policy
-- @param string "expand" or "master_width_factor"

function tag.object.get_master_fill_policy(t)
    return tag.getproperty(t, "master_fill_policy")
        or beautiful.master_fill_policy
        or defaults.master_fill_policy
end

--- Set size fill policy for the master client(s)
-- @deprecated awful.tag.setmfpol
-- @see master_fill_policy
-- @tparam string policy Can be set to
-- "expand" (fill all the available workarea) or
-- "master_width_factor" (fill only an area inside the master width factor)
-- @tparam[opt=tag.selected()] tag t The tag to modify
function tag.setmfpol(policy, t)
    gdebug.deprecate("Use t.master_fill_policy = policy instead of awful.tag.setmfpol", {deprecated_in=4})

    t = t or ascreen.focused().selected_tag
    tag.setproperty(t, "master_fill_policy", policy)
end

--- Toggle size fill policy for the master client(s)
-- between "expand" and "master_width_factor".
-- @function awful.tag.togglemfpol
-- @see master_fill_policy
-- @tparam tag t The tag to modify, if null tag.selected() is used.
function tag.togglemfpol(t)
    t = t or ascreen.focused().selected_tag

    if tag.getmfpol(t) == "expand" then
        tag.setproperty(t, "master_fill_policy", "master_width_factor")
    else
        tag.setproperty(t, "master_fill_policy", "expand")
    end
end

--- Get size fill policy for the master client(s)
-- @deprecated awful.tag.getmfpol
-- @see master_fill_policy
-- @tparam[opt=tag.selected()] tag t The tag
-- @treturn string Possible values are
-- "expand" (fill all the available workarea, default one) or
-- "master_width_factor" (fill only an area inside the master width factor)
function tag.getmfpol(t)
    gdebug.deprecate("Use t.master_fill_policy instead of awful.tag.getmfpol", {deprecated_in=4})

    t = t or ascreen.focused().selected_tag
    return tag.getproperty(t, "master_fill_policy")
        or beautiful.master_fill_policy
        or defaults.master_fill_policy
end

--- The default number of master windows.
--
-- @beautiful beautiful.master_count
-- @param integer (default: 1)
-- @see master_count

--- Set the number of master windows.
--
-- **Signal:**
--
-- * *property::nmaster* (deprecated)
-- * *property::master_count*
--
-- @property master_count
-- @param integer nmaster Only positive values are accepted

function tag.object.set_master_count(t, nmaster)
    if nmaster >= 0 then
        tag.setproperty(t, "nmaster", nmaster)
        tag.setproperty(t, "master_count", nmaster)
    end
end

function tag.object.get_master_count(t)
    return tag.getproperty(t, "master_count")
        or beautiful.master_count
        or defaults.master_count
end

---
-- @deprecated awful.tag.setnmaster
-- @see master_count
-- @param nmaster The number of master windows.
-- @param[opt] t The tag.
function tag.setnmaster(nmaster, t)
    gdebug.deprecate("Use t.master_count = nmaster instead of awful.tag.setnmaster", {deprecated_in=4})

    tag.object.set_master_count(t or ascreen.focused().selected_tag, nmaster)
end

--- Get the number of master windows.
-- @deprecated awful.tag.getnmaster
-- @see master_count
-- @param[opt] t The tag.
function tag.getnmaster(t)
    gdebug.deprecate("Use t.master_count instead of awful.tag.setnmaster", {deprecated_in=4})

    t = t or ascreen.focused().selected_tag
    return tag.getproperty(t, "master_count") or 1
end

--- Increase the number of master windows.
-- @function awful.tag.incnmaster
-- @see master_count
-- @param add Value to add to number of master windows.
-- @param[opt] t The tag to modify, if null tag.selected() is used.
-- @tparam[opt=false] boolean sensible Limit nmaster based on the number of
--   visible tiled windows?
function tag.incnmaster(add, t, sensible)
    t = t or ascreen.focused().selected_tag

    if sensible then
        local screen = get_screen(tag.getproperty(t, "screen"))
        local ntiled = #screen.tiled_clients

        local nmaster = tag.object.get_master_count(t)
        if nmaster > ntiled then
            nmaster = ntiled
        end

        local newnmaster = nmaster + add
        if newnmaster > ntiled then
            newnmaster = ntiled
        end
        tag.object.set_master_count(t, newnmaster)
    else
        tag.object.set_master_count(t, tag.object.get_master_count(t) + add)
    end
end

--- Set the tag icon.
--
-- **Signal:**
--
-- * *property::icon*
--
-- @property icon
-- @tparam path|surface icon The icon

-- accessors are implicit.

--- Set the tag icon
-- @deprecated awful.tag.seticon
-- @see icon
-- @param icon the icon to set, either path or image object
-- @param _tag the tag
function tag.seticon(icon, _tag)
    gdebug.deprecate("Use t.icon = icon instead of awful.tag.seticon", {deprecated_in=4})

    _tag = _tag or ascreen.focused().selected_tag
    tag.setproperty(_tag, "icon", icon)
end

--- Get the tag icon
-- @deprecated awful.tag.geticon
-- @see icon
-- @param _tag the tag
function tag.geticon(_tag)
    gdebug.deprecate("Use t.icon instead of awful.tag.geticon", {deprecated_in=4})

    _tag = _tag or ascreen.focused().selected_tag
    return tag.getproperty(_tag, "icon")
end

--- The default number of columns.
--
-- @beautiful beautiful.column_count
-- @param integer (default: 1)
-- @see column_count

--- Set the number of columns.
--
-- **Signal:**
--
-- * *property::ncol* (deprecated)
-- * *property::column_count*
--
-- @property column_count
-- @tparam integer ncol Has to be greater than 1

function tag.object.set_column_count(t, ncol)
    if ncol >= 1 then
        tag.setproperty(t, "ncol", ncol)
        tag.setproperty(t, "column_count", ncol)
    end
end

function tag.object.get_column_count(t)
    return tag.getproperty(t, "column_count")
        or beautiful.column_count
        or defaults.column_count
end

--- Set number of column windows.
-- @deprecated awful.tag.setncol
-- @see column_count
-- @param ncol The number of column.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.setncol(ncol, t)
    gdebug.deprecate("Use t.column_count = new_index instead of awful.tag.setncol", {deprecated_in=4})

    t = t or ascreen.focused().selected_tag
    if ncol >= 1 then
        tag.setproperty(t, "ncol", ncol)
        tag.setproperty(t, "column_count", ncol)
    end
end

--- Get number of column windows.
-- @deprecated awful.tag.getncol
-- @see column_count
-- @param[opt] t The tag.
function tag.getncol(t)
    gdebug.deprecate("Use t.column_count instead of awful.tag.getncol", {deprecated_in=4})

    t = t or ascreen.focused().selected_tag
    return tag.getproperty(t, "column_count") or 1
end

--- Increase number of column windows.
-- @function awful.tag.incncol
-- @param add Value to add to number of column windows.
-- @param[opt] t The tag to modify, if null tag.selected() is used.
-- @tparam[opt=false] boolean sensible Limit column_count based on the number
--   of visible tiled windows?
function tag.incncol(add, t, sensible)
    t = t or ascreen.focused().selected_tag

    if sensible then
        local screen = get_screen(tag.getproperty(t, "screen"))
        local ntiled = #screen.tiled_clients
        local nmaster = tag.object.get_master_count(t)
        local nsecondary = ntiled - nmaster

        local ncol = tag.object.get_column_count(t)
        if ncol > nsecondary then
            ncol = nsecondary
        end

        local newncol = ncol + add
        if newncol > nsecondary then
            newncol = nsecondary
        end

        tag.object.set_column_count(t, newncol)
    else
        tag.object.set_column_count(t, tag.object.get_column_count(t) + add)
    end
end

--- View no tag.
-- @function awful.tag.viewnone
-- @tparam[opt] int|screen screen The screen.
function tag.viewnone(screen)
    screen = screen or ascreen.focused()
    local tags = screen.tags
    for _, t in pairs(tags) do
        t.selected = false
    end
end

--- View a tag by its taglist index.
--
-- This is equivalent to `screen.tags[i]:view_only()`
-- @function awful.tag.viewidx
-- @see screen.tags
-- @param i The **relative** index to see.
-- @param[opt] screen The screen.
function tag.viewidx(i, screen)
    screen = get_screen(screen or ascreen.focused())
    local tags = screen.tags
    local showntags = {}
    for _, t in ipairs(tags) do
        if not tag.getproperty(t, "hide") then
            table.insert(showntags, t)
        end
    end
    local sel = screen.selected_tag
    tag.viewnone(screen)
    for k, t in ipairs(showntags) do
        if t == sel then
            showntags[gmath.cycle(#showntags, k + i)].selected = true
        end
    end
    screen:emit_signal("tag::history::update")
end

--- Get a tag's index in the gettags() table.
-- @deprecated awful.tag.getidx
-- @see index
-- @param query_tag The tag object to find. [selected()]
-- @return The index of the tag, nil if the tag is not found.
function tag.getidx(query_tag)
    gdebug.deprecate("Use t.index instead of awful.tag.getidx", {deprecated_in=4})

    return tag.object.get_index(query_tag or ascreen.focused().selected_tag)
end

--- View next tag. This is the same as tag.viewidx(1).
-- @function awful.tag.viewnext
-- @param screen The screen.
function tag.viewnext(screen)
    return tag.viewidx(1, screen)
end

--- View previous tag. This is the same a tag.viewidx(-1).
-- @function awful.tag.viewprev
-- @param screen The screen.
function tag.viewprev(screen)
    return tag.viewidx(-1, screen)
end

--- View only a tag.
-- @function tag.view_only
-- @see selected
function tag.object.view_only(self)
    local tags = self.screen.tags
    -- First, untag everyone except the viewed tag.
    for _, _tag in pairs(tags) do
        if _tag ~= self then
            _tag.selected = false
        end
    end
    -- Then, set this one to selected.
    -- We need to do that in 2 operations so we avoid flickering and several tag
    -- selected at the same time.
    self.selected = true
    capi.screen[self.screen]:emit_signal("tag::history::update")
end

--- View only a tag.
-- @deprecated awful.tag.viewonly
-- @see tag.view_only
-- @param t The tag object.
function tag.viewonly(t)
    gdebug.deprecate("Use t:view_only() instead of awful.tag.viewonly", {deprecated_in=4})

    tag.object.view_only(t)
end

--- View only a set of tags.
--
-- If `maximum` is set, there will be a limit on the number of new tag being
-- selected. The tags already selected do not count. To do nothing if one or
-- more of the tags are already selected, set `maximum` to zero.
--
-- @function awful.tag.viewmore
-- @param tags A table with tags to view only.
-- @param[opt] screen The screen of the tags.
-- @tparam[opt=#tags] number maximum The maximum number of tags to select.
function tag.viewmore(tags, screen, maximum)
    maximum = maximum or #tags
    local selected = 0
    screen = get_screen(screen or ascreen.focused())
    local screen_tags = screen.tags
    for _, _tag in ipairs(screen_tags) do
        if not gtable.hasitem(tags, _tag) then
            _tag.selected = false
        elseif _tag.selected then
            selected = selected + 1
        end
    end
    for _, _tag in ipairs(tags) do
        if selected == 0 and maximum == 0 then
            _tag.selected = true
            break
        end

        if selected >= maximum then break end

        if not _tag.selected then
            selected = selected + 1
            _tag.selected = true
        end
    end
    screen:emit_signal("tag::history::update")
end

--- Toggle selection of a tag
-- @function awful.tag.viewtoggle
-- @see selected
-- @tparam tag t Tag to be toggled
function tag.viewtoggle(t)
    t.selected = not t.selected
    capi.screen[tag.getproperty(t, "screen")]:emit_signal("tag::history::update")
end

--- Get tag data table.
--
-- Do not use.
--
-- @deprecated awful.tag.getdata
-- @tparam tag _tag The tag.
-- @return The data table.
function tag.getdata(_tag)
    return _tag.data.awful_tag_properties
end

--- Get a tag property.
--
-- Use `_tag.prop` directly.
--
-- @deprecated awful.tag.getproperty
-- @tparam tag _tag The tag.
-- @tparam string prop The property name.
-- @return The property.
function tag.getproperty(_tag, prop)
    if not _tag then return end -- FIXME: Turn this into an error?
    if _tag.data.awful_tag_properties then
       return _tag.data.awful_tag_properties[prop]
    end
end

--- Set a tag property.
-- This properties are internal to awful. Some are used to draw taglist, or to
-- handle layout, etc.
--
-- Use `_tag.prop = value`
--
-- @deprecated awful.tag.setproperty
-- @param _tag The tag.
-- @param prop The property name.
-- @param value The value.
function tag.setproperty(_tag, prop, value)
    if not _tag.data.awful_tag_properties then
        _tag.data.awful_tag_properties = {}
    end

    if _tag.data.awful_tag_properties[prop] ~= value then
        _tag.data.awful_tag_properties[prop] = value
        _tag:emit_signal("property::" .. prop)
    end
end

--- Tag a client with the set of current tags.
-- @deprecated awful.tag.withcurrent
-- @param c The client to tag.
function tag.withcurrent(c)
    gdebug.deprecate("Use c:to_selected_tags() instead of awful.tag.selectedlist", {deprecated_in=4})

    -- It can't use c:to_selected_tags() because awful.tag is loaded before
    -- awful.client

    local tags = {}
    for _, t in ipairs(c:tags()) do
        if get_screen(tag.getproperty(t, "screen")) == get_screen(c.screen) then
            table.insert(tags, t)
        end
    end
    if #tags == 0 then
        tags = c.screen.selected_tags
    end
    if #tags == 0 then
        tags = c.screen.tags
    end
    if #tags ~= 0 then
        c:tags(tags)
    end
end

local function attached_connect_signal_screen(screen, sig, func)
    screen = get_screen(screen)
    capi.tag.connect_signal(sig, function(_tag)
        if get_screen(tag.getproperty(_tag, "screen")) == screen then
            func(_tag)
        end
    end)
end

--- Add a signal to all attached tags and all tags that will be attached in the
-- future. When a tag is detached from the screen, its signal is removed.
--
-- @function awful.tag.attached_connect_signal
-- @screen screen The screen concerned, or all if nil.
-- @tparam[opt] string signal The signal name.
-- @tparam[opt] function Callback
function tag.attached_connect_signal(screen, ...)
    if screen then
        attached_connect_signal_screen(screen, ...)
    else
        capi.tag.connect_signal(...)
    end
end

-- Register standard signals.
capi.client.connect_signal("property::screen", function(c)
    -- First, the delayed timer is necessary to avoid a race condition with
    -- awful.rules. It is also messing up the tags before the user have a chance
    -- to set them manually.
    timer.delayed_call(function()
        if not c.valid then
            return
        end
        local tags, new_tags = c:tags(), {}

        for _, t in ipairs(tags) do
            if t.screen == c.screen then
                table.insert(new_tags, t)
            end
        end

        if #new_tags == 0 then
            c:emit_signal("request::tag", nil, {reason="screen"})
        elseif #new_tags < #tags then
            c:tags(new_tags)
        end
    end)
end)

-- Keep track of the number of urgent clients.
local function update_urgent(t, modif)
    local count = tag.getproperty(t, "urgent_count") or 0
    count = (count + modif) >= 0 and (count + modif) or 0
    tag.setproperty(t, "urgent"      , count > 0)
    tag.setproperty(t, "urgent_count", count    )
end

-- Update the urgent counter when a client is tagged.
local function client_tagged(c, t)
    if c.urgent then
        update_urgent(t, 1)
    end
end

-- Update the urgent counter when a client is untagged.
local function client_untagged(c, t)
    if c.urgent then
        update_urgent(t, -1)
    end

    if #t:clients() == 0 and tag.getproperty(t, "volatile") then
        tag.object.delete(t)
    end
end

-- Count the urgent clients.
local function urgent_callback(c)
    for _,t in ipairs(c:tags()) do
        update_urgent(t, c.urgent and 1 or -1)
    end
end

capi.client.connect_signal("property::urgent", urgent_callback)
capi.client.connect_signal("untagged", client_untagged)
capi.client.connect_signal("tagged", client_tagged)
capi.tag.connect_signal("request::select", tag.object.view_only)

--- True when a tagged client is urgent
-- @signal property::urgent
-- @see client.urgent

--- The number of urgent tagged clients
-- @signal property::urgent_count
-- @see client.urgent

--- Emitted when a screen is removed.
-- This can be used to salvage existing tags by moving them to a new
-- screen (or creating a virtual screen). By default, there is no
-- handler for this request. The tags will be deleted. To prevent
-- this, an handler for this request must simply set a new screen
-- for the tag.
-- @signal request::screen

--- Emitted after `request::screen` if no new screen has been set.
-- The tag will be deleted, this is a last chance to move its clients
-- before they are sent to a fallback tag. Connect to `request::screen`
-- if you wish to salvage the tag.
-- @signal removal-pending

capi.screen.connect_signal("tag::history::update", tag.history.update)

-- Make sure the history is set early
timer.delayed_call(function()
    for s in capi.screen do
        s:emit_signal("tag::history::update")
    end
end)

capi.screen.connect_signal("removed", function(s)
    -- First give other code a chance to move the tag to another screen
    for _, t in pairs(s.tags) do
        t:emit_signal("request::screen")
    end
    -- Everything that's left: Tell everyone that these tags go away (other code
    -- could e.g. save clients)
    for _, t in pairs(s.tags) do
        t:emit_signal("removal-pending")
    end
    -- Give other code yet another change to save clients
    for _, c in pairs(capi.client.get(s)) do
        c:emit_signal("request::tag", nil, { reason = "screen-removed" })
    end
    -- Then force all clients left to go somewhere random
    local fallback = nil
    for other_screen in capi.screen do
        if #other_screen.tags > 0 then
            fallback = other_screen.tags[1]
            break
        end
    end
    for _, t in pairs(s.tags) do
        t:delete(fallback, true)
    end
    -- If any tag survived until now, forcefully get rid of it
    for _, t in pairs(s.tags) do
        t.activated = false

        if t.data.awful_tag_properties then
            t.data.awful_tag_properties.screen = nil
        end
    end
end)

function tag.mt:__call(...)
    return tag.new(...)
end

-- Extend the luaobject
-- `awful.tag.setproperty` currently handle calling the setter method itself
-- while `awful.tag.getproperty`.
object.properties(capi.tag, {
    getter_class    = tag.object,
    setter_class    = tag.object,
    getter_fallback = tag.getproperty,
    setter_fallback = tag.setproperty,
})

return setmetatable(tag, tag.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
