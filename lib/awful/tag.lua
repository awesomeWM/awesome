---------------------------------------------------------------------------
--- Useful functions for tag manipulation.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.tag
---------------------------------------------------------------------------

-- Grab environment we need
local util = require("awful.util")
local ascreen = require("awful.screen")
local beautiful = require("beautiful")
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

-- we use require("awful.client") inside functions to prevent circular dependencies.
local client

local tag = { mt = {} }

-- Private data
local data = {}
data.history = {}
data.dynamic_cache = setmetatable({}, { __mode = 'k' })
data.tags = setmetatable({}, { __mode = 'k' })

-- History functions
tag.history = {}
tag.history.limit = 20

--- Move a tag to an absolute position in the screen[]:tags() table.
-- @param new_index Integer absolute position in the table to insert.
-- @param target_tag The tag that should be moved. If null, the currently
-- selected tag is used.
function tag.move(new_index, target_tag)
    target_tag = target_tag or tag.selected()
    local scr = get_screen(tag.getscreen(target_tag))
    local tmp_tags = tag.gettags(scr)

    if (not new_index) or (new_index < 1) or (new_index > #tmp_tags) then
        return
    end

    local rm_index = nil

    for i, t in ipairs(tmp_tags) do
        if t == target_tag then
            table.remove(tmp_tags, i)
            rm_index = i
            break
        end
    end

    table.insert(tmp_tags, new_index, target_tag)

    for i=new_index < rm_index and new_index or rm_index, #tmp_tags do
        local tmp_tag = tmp_tags[i]
        tag.setscreen(scr, tmp_tag)
        tag.setproperty(tmp_tag, "index", i)
    end
end

--- Swap 2 tags
-- @param tag1 The first tag
-- @param tag2 The second tag
function tag.swap(tag1, tag2)
    local idx1, idx2 = tag.getidx(tag1), tag.getidx(tag2)
    local src2, src1 = tag.getscreen(tag2), tag.getscreen(tag1)
    tag.setscreen(src1, tag2)
    tag.move(idx1, tag2)
    tag.setscreen(src2, tag1)
    tag.move(idx2, tag1)
end

--- Add a tag.
-- @param name The tag name, a string
-- @param props The tags inital properties, a table
-- @return The created tag
function tag.add(name, props)
    local properties = props or {}

    -- Be sure to set the screen before the tag is activated to avoid function
    -- connected to property::activated to be called without a valid tag.
    -- set properties cannot be used as this has to be set before the first
    -- signal is sent
    properties.screen = get_screen(properties.screen or ascreen.focused())

    -- Index is also required
    properties.index = (#tag.gettags(properties.screen))+1

    local newtag = capi.tag{ name = name }

    -- Start with a fresh property table to avoid collisions with unsupported data
    data.tags[newtag] = {screen=properties.screen, index=properties.index}

    newtag.activated = true

    for k, v in pairs(properties) do
        tag.setproperty(newtag, k, v)
    end

    return newtag
end

--- Create a set of tags and attach it to a screen.
-- @param names The tag name, in a table
-- @param screen The tag screen, or 1 if not set.
-- @param layout The layout or layout table to set for this tags by default.
-- @return A table with all created tags.
function tag.new(names, screen, layout)
    screen = get_screen(screen or 1)
    local tags = {}
    for id, name in ipairs(names) do
        table.insert(tags, id, tag.add(name, {screen = screen,
                                            layout = (layout and layout[id]) or
                                                        layout}))
        -- Select the first tag.
        if id == 1 then
            tags[id].selected = true
        end
    end

    return tags
end

--- Find a suitable fallback tag.
-- @param screen The screen to look for a tag on. [awful.screen.focused()]
-- @param invalids A table of tags we consider unacceptable. [selectedlist(scr)]
function tag.find_fallback(screen, invalids)
    local scr = screen or ascreen.focused()
    local t = invalids or tag.selectedlist(scr)

    for _, v in pairs(tag.gettags(scr)) do
        if not util.table.hasitem(t, v) then return v end
    end
end

--- Delete a tag.
-- @param target_tag Optional tag object to delete. [selected()]
-- @param fallback_tag Tag to assign stickied tags to. [~selected()]
-- @return Returns true if the tag is successfully deleted, nil otherwise.
-- If there are no clients exclusively on this tag then delete it. Any
-- stickied clients are assigned to the optional 'fallback_tag'.
-- If after deleting the tag there is no selected tag, try and restore from
-- history or select the first tag on the screen.
function tag.delete(target_tag, fallback_tag)
    -- abort if no tag is passed or currently selected
    target_tag = target_tag or tag.selected()
    if target_tag == nil or target_tag.activated == false then return end

    local target_scr = get_screen(tag.getscreen(target_tag))
    local tags       = tag.gettags(target_scr)
    local idx        = tag.getidx(target_tag)
    local ntags      = #tags

    -- We can't use the target tag as a fallback.
    if fallback_tag == target_tag then return end

    -- No fallback_tag provided, try and get one.
    if fallback_tag == nil then
        fallback_tag = tag.find_fallback(target_scr, {target_tag})
    end

    -- Abort if we would have un-tagged clients.
    local clients = target_tag:clients()
    if ( #clients > 0 and ntags <= 1 ) or fallback_tag == nil then return end

    -- Move the clients we can off of this tag.
    for _, c in pairs(clients) do
        local nb_tags = #c:tags()

        -- If a client has only this tag, or stickied clients with
        -- nowhere to go, abort.
        if (not c.sticky and nb_tags == 1) or
                                    (c.sticky and fallback_tag == nil) then
            return
        -- If a client has multiple tags, then do not move it to fallback
        elseif nb_tags < 2 then
            c:tags({fallback_tag})
        end
    end

    -- delete the tag
    data.tags[target_tag].screen = nil
    target_tag.activated = false

    -- Update all indexes
    for i=idx+1, #tags do
        tag.setproperty(tags[i], "index", i-1)
    end

    -- If no tags are visible, try and view one.
    if tag.selected(target_scr) == nil and ntags > 0 then
        tag.history.restore(nil, 1)
        if tag.selected(target_scr) == nil then
            tags[tags[1] == target_tag and 2 or 1].selected = true
        end
    end

    return true
end

--- Update the tag history.
-- @param obj Screen object.
function tag.history.update(obj)
    local s = get_screen(obj)
    local curtags = tag.selectedlist(s)
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
-- @param screen The screen.
-- @param idx Index in history. Defaults to "previous" which is a special index
-- toggling between last two selected sets of tags. Number (eg 1) will go back
-- to the given index in history.
function tag.history.restore(screen, idx)
    local s = get_screen(screen or ascreen.focused())
    local i = idx or "previous"
    local sel = tag.selectedlist(s)
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
        t.selected = true
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
-- @param s Screen
-- @return A table with all available tags
function tag.gettags(s)
    s = get_screen(s)
    local tags = {}
    for _, t in ipairs(root.tags()) do
        if get_screen(tag.getscreen(t)) == s then
            table.insert(tags, t)
        end
    end

    table.sort(tags, function(a, b)
        return (tag.getproperty(a, "index") or 9999) < (tag.getproperty(b, "index") or 9999)
    end)
    return tags
end

--- Set a tag's screen
-- @param s Screen
-- @param t tag object
function tag.setscreen(s, t)

    -- For API consistency, the arguments have been swapped for Awesome 3.6
    if type(t) == "number" then
        util.deprecate("tag.setscreen arguments are now (s, t) instead of (t, s)")
        s, t = t, s
    end

    s = get_screen(s or ascreen.focused())
    local sel = tag.selected
    local old_screen = tag.getproperty(t, "screen")
    if s == old_screen then return end

    -- Keeping the old index make very little sense when changing screen
    tag.setproperty(t, "index", nil, true)

    -- Change the screen
    tag.setproperty(t, "screen", s, true)

    -- Make sure the client's screen matches its tags
    for _,c in ipairs(t:clients()) do
        c.screen = s --Move all clients
        c:tags({t})
    end

    -- Update all indexes
    for _,screen in ipairs {old_screen, s} do
        for i,t2 in ipairs(tag.gettags(screen)) do
            tag.setproperty(t2, "index", i, true)
        end
    end

    -- Restore the old screen history if the tag was selected
    if sel then
        tag.history.restore(old_screen, 1)
    end
end

--- Get a tag's screen
-- @param[opt] t tag object
-- @return Screen number
function tag.getscreen(t)
    t = t or tag.selected()
    local prop = tag.getproperty(t, "screen")
    return prop and prop.index
end

--- Return a table with all visible tags
-- @param s Screen.
-- @return A table with all selected tags.
function tag.selectedlist(s)
    local screen = s or ascreen.focused()
    local tags = tag.gettags(screen)
    local vtags = {}
    for _, t in pairs(tags) do
        if t.selected then
            vtags[#vtags + 1] = t
        end
    end
    return vtags
end

--- Return only the first visible tag.
-- @param s Screen.
function tag.selected(s)
    return tag.selectedlist(s)[1]
end

--- Set master width factor.
-- @param mwfact Master width factor.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.setmwfact(mwfact, t)
    t = t or tag.selected()
    if mwfact >= 0 and mwfact <= 1 then
        tag.setproperty(t, "mwfact", mwfact, true)
    end
end

--- Increase master width factor.
-- @param add Value to add to master width factor.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.incmwfact(add, t)
    tag.setmwfact(tag.getmwfact(t) + add, t)
end

--- Get master width factor.
-- @param[opt] t The tag.
function tag.getmwfact(t)
    t = t or tag.selected()
    return tag.getproperty(t, "mwfact") or 0.5
end

--- Set layout
-- @param layout a layout table or a constructor function
-- @param t The tag to modify
-- @return The layout
function tag.setlayout(layout, t)

    -- Check if the signature match a stateful layout
    if type(layout) == "function" or (
        type(layout) == "table"
        and getmetatable(layout)
        and getmetatable(layout).__call
    ) then
        if not data.dynamic_cache[t] then
            data.dynamic_cache[t] = {}
        end

        local instance = data.dynamic_cache[t][layout] or layout(t)

        -- Always make sure the layout is notified it is enabled
        if tag.selected(tag.getscreen(t)) == t and instance.wake_up then
            instance:wake_up()
        end

        -- Avoid creating the same layout twice, use layout:reset() to reset
        if instance.is_dynamic then
            data.dynamic_cache[t][layout] = instance
        end

        layout = instance
    end

    tag.setproperty(t, "layout", layout, true)

    return layout
end

--- Set the spacing between clients
-- @param useless_gap The spacing between clients
-- @param t The tag to modify, if null tag.selected() is used.
function tag.setgap(useless_gap, t)
    t = t or tag.selected()
    if useless_gap >= 0 then
        tag.setproperty(t, "useless_gap", useless_gap, true)
    end
end

--- Increase the spacing between clients
-- @param add Value to add to the spacing between clients
-- @param t The tag to modify, if null tag.selected() is used.
function tag.incgap(add, t)
    tag.setgap(tag.getgap(t) + add, t)
end

--- Get the spacing between clients.
-- @tparam[opt=tag.selected()] tag t The tag.
-- @tparam[opt] int numclients Number of (tiled) clients.  Passing this will
--   return 0 for a single client.  You can override this function to change
--   this behavior.
function tag.getgap(t, numclients)
    t = t or tag.selected()
    if numclients == 1 then
        return 0
    end
    return tag.getproperty(t, "useless_gap") or beautiful.useless_gap or 0
end

--- Set size fill policy for the master client(s)
-- @tparam string policy Can be set to
-- "expand" (fill all the available workarea) or
-- "mwfact" (fill only an area inside the master width factor)
-- @tparam[opt=tag.selected()] tag t The tag to modify
function tag.setmfpol(policy, t)
    t = t or tag.selected()
    tag.setproperty(t, "master_fill_policy", policy, true)
end

--- Toggle size fill policy for the master client(s)
-- between "expand" and "mwfact"
-- @tparam tag t The tag to modify, if null tag.selected() is used.
function tag.togglemfpol(t)
    if tag.getmfpol(t) == "expand" then
        tag.setmfpol("mwfact", t)
    else
        tag.setmfpol("expand", t)
    end
end

--- Get size fill policy for the master client(s)
-- @tparam[opt=tag.selected()] tag t The tag
-- @treturn string Possible values are
-- "expand" (fill all the available workarea, default one) or
-- "mwfact" (fill only an area inside the master width factor)
function tag.getmfpol(t)
    t = t or tag.selected()
    return tag.getproperty(t, "master_fill_policy") or "expand"
end

--- Set the number of master windows.
-- @param nmaster The number of master windows.
-- @param[opt] t The tag.
function tag.setnmaster(nmaster, t)
    t = t or tag.selected()
    if nmaster >= 0 then
        tag.setproperty(t, "nmaster", nmaster, true)
    end
end

--- Get the number of master windows.
-- @param[opt] t The tag.
function tag.getnmaster(t)
    t = t or tag.selected()
    return tag.getproperty(t, "nmaster") or 1
end

--- Increase the number of master windows.
-- @param add Value to add to number of master windows.
-- @param[opt] t The tag to modify, if null tag.selected() is used.
-- @tparam[opt=false] boolean sensible Limit nmaster based on the number of
--   visible tiled windows?
function tag.incnmaster(add, t, sensible)
    if sensible then
        client = client or require("awful.client")
        local screen = get_screen(tag.getscreen(t))
        local ntiled = #client.tiled(screen)

        local nmaster = tag.getnmaster(t)
        if nmaster > ntiled then
            nmaster = ntiled
        end

        local newnmaster = nmaster + add
        if newnmaster > ntiled then
            newnmaster = ntiled
        end
        tag.setnmaster(newnmaster, t)
    else
        tag.setnmaster(tag.getnmaster(t) + add, t)
    end
end


--- Set the tag icon
-- @param icon the icon to set, either path or image object
-- @param _tag the tag
function tag.seticon(icon, _tag)
    _tag = _tag or tag.selected()
    tag.setproperty(_tag, "icon", icon, true)
end

--- Get the tag icon
-- @param _tag the tag
function tag.geticon(_tag)
    _tag = _tag or tag.selected()
    return tag.getproperty(_tag, "icon")
end

--- Set number of column windows.
-- @param ncol The number of column.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.setncol(ncol, t)
    t = t or tag.selected()
    if ncol >= 1 then
        tag.setproperty(t, "ncol", ncol, true)
    end
end

--- Get number of column windows.
-- @param[opt] t The tag.
function tag.getncol(t)
    t = t or tag.selected()
    return tag.getproperty(t, "ncol") or 1
end

--- Increase number of column windows.
-- @param add Value to add to number of column windows.
-- @param[opt] t The tag to modify, if null tag.selected() is used.
-- @tparam[opt=false] boolean sensible Limit ncol based on the number of visible
-- tiled windows?
function tag.incncol(add, t, sensible)
    if sensible then
        client = client or require("awful.client")
        local screen = get_screen(tag.getscreen(t))
        local ntiled = #client.tiled(screen)
        local nmaster = tag.getnmaster(t)
        local nsecondary = ntiled - nmaster

        local ncol = tag.getncol(t)
        if ncol > nsecondary then
            ncol = nsecondary
        end

        local newncol = ncol + add
        if newncol > nsecondary then
            newncol = nsecondary
        end
        tag.setncol(newncol, t)
    else
        tag.setncol(tag.getncol(t) + add, t)
    end
end

--- View no tag.
-- @tparam[opt] int|screen screen The screen.
function tag.viewnone(screen)
    local tags = tag.gettags(screen or ascreen.focused())
    for _, t in pairs(tags) do
        t.selected = false
    end
end

--- View a tag by its taglist index.
-- @param i The relative index to see.
-- @param[opt] screen The screen.
function tag.viewidx(i, screen)
    screen = get_screen(screen or ascreen.focused())
    local tags = tag.gettags(screen)
    local showntags = {}
    for _, t in ipairs(tags) do
        if not tag.getproperty(t, "hide") then
            table.insert(showntags, t)
        end
    end
    local sel = tag.selected(screen)
    tag.viewnone(screen)
    for k, t in ipairs(showntags) do
        if t == sel then
            showntags[util.cycle(#showntags, k + i)].selected = true
        end
    end
    screen:emit_signal("tag::history::update")
end

--- Get a tag's index in the gettags() table.
-- @param query_tag The tag object to find. [selected()]
-- @return The index of the tag, nil if the tag is not found.
function tag.getidx(query_tag)
    query_tag = query_tag or tag.selected()
    if query_tag == nil then return end

    for i, t in ipairs(tag.gettags(tag.getscreen(query_tag))) do
        if t == query_tag then
            return i
        end
    end
end

--- View next tag. This is the same as tag.viewidx(1).
-- @param screen The screen.
function tag.viewnext(screen)
    return tag.viewidx(1, screen)
end

--- View previous tag. This is the same a tag.viewidx(-1).
-- @param screen The screen.
function tag.viewprev(screen)
    return tag.viewidx(-1, screen)
end

--- View only a tag.
-- @param t The tag object.
function tag.viewonly(t)
    local tags = tag.gettags(tag.getscreen(t))
    -- First, untag everyone except the viewed tag.
    for _, _tag in pairs(tags) do
        if _tag ~= t then
            _tag.selected = false
        end
    end
    -- Then, set this one to selected.
    -- We need to do that in 2 operations so we avoid flickering and several tag
    -- selected at the same time.
    t.selected = true
    capi.screen[tag.getscreen(t)]:emit_signal("tag::history::update")
end

--- View only a set of tags.
-- @param tags A table with tags to view only.
-- @param[opt] screen The screen of the tags.
function tag.viewmore(tags, screen)
    screen = get_screen(screen or ascreen.focused())
    local screen_tags = tag.gettags(screen)
    for _, _tag in ipairs(screen_tags) do
        if not util.table.hasitem(tags, _tag) then
            _tag.selected = false
        end
    end
    for _, _tag in ipairs(tags) do
        _tag.selected = true
    end
    screen:emit_signal("tag::history::update")
end

--- Toggle selection of a tag
-- @tparam tag t Tag to be toggled
function tag.viewtoggle(t)
    t.selected = not t.selected
    capi.screen[tag.getscreen(t)]:emit_signal("tag::history::update")
end

--- Get tag data table.
-- @tparam tag _tag The tag.
-- @return The data table.
function tag.getdata(_tag)
    return data.tags[_tag]
end

--- Get a tag property.
-- @tparam tag _tag The tag.
-- @tparam string prop The property name.
-- @return The property.
function tag.getproperty(_tag, prop)
    if data.tags[_tag] then
        return data.tags[_tag][prop]
    end
end

--- Set a tag property.
-- This properties are internal to awful. Some are used to draw taglist, or to
-- handle layout, etc.
-- @param _tag The tag.
-- @param prop The property name.
-- @param value The value.
-- @param ignore_setters Ignore the setter function for "prop" (boolean)
function tag.setproperty(_tag, prop, value, ignore_setters)
    if not data.tags[_tag] then
        data.tags[_tag] = {}
    end

    if (not ignore_setters) and tag["set"..prop] then
        tag["set"..prop](value, _tag)
    else
        if data.tags[_tag][prop] ~= value then
            data.tags[_tag][prop] = value
            _tag:emit_signal("property::" .. prop)
        end
    end
end

--- Tag a client with the set of current tags.
-- @param c The client to tag.
function tag.withcurrent(c)
    local tags = {}
    for _, t in ipairs(c:tags()) do
        if get_screen(tag.getscreen(t)) == get_screen(c.screen) then
            table.insert(tags, t)
        end
    end
    if #tags == 0 then
        tags = tag.selectedlist(c.screen)
    end
    if #tags == 0 then
        tags = tag.gettags(c.screen)
    end
    if #tags ~= 0 then
        c:tags(tags)
    end
end

local function attached_connect_signal_screen(screen, sig, func)
    screen = get_screen(screen)
    capi.tag.connect_signal(sig, function(_tag)
        if get_screen(tag.getscreen(_tag)) == screen then
            func(_tag)
        end
    end)
end

--- Add a signal to all attached tags and all tags that will be attached in the
-- future. When a tag is detached from the screen, its signal is removed.
-- @param screen The screen concerned, or all if nil.
function tag.attached_connect_signal(screen, ...)
    if screen then
        attached_connect_signal_screen(screen, ...)
    else
        capi.tag.connect_signal(...)
    end
end

-- Register standard signals.
capi.client.connect_signal("manage", function(c)
    -- If we are not managing this application at startup,
    -- move it to the screen where the mouse is.
    -- We only do it for "normal" windows (i.e. no dock, etc).
    if not awesome.startup and c.type ~= "desktop" and c.type ~= "dock" then
        if c.transient_for then
            c.screen = c.transient_for.screen
            if not c.sticky then
                c:tags(c.transient_for:tags())
            end
        else
            c.screen = ascreen.focused()
        end
    end
    c:connect_signal("property::screen", tag.withcurrent)
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
end

-- Count the urgent clients.
local function urgent_callback(c)
    for _,t in ipairs(c:tags()) do
        update_urgent(t, c.urgent and 1 or -1)
    end
end

capi.client.connect_signal("property::urgent", urgent_callback)
capi.client.connect_signal("manage", tag.withcurrent)
capi.client.connect_signal("untagged", client_untagged)
capi.client.connect_signal("tagged", client_tagged)
capi.tag.connect_signal("request::select", tag.viewonly)

capi.tag.add_signal("property::hide")
capi.tag.add_signal("property::icon")
capi.tag.add_signal("property::icon_only")
capi.tag.add_signal("property::layout")
capi.tag.add_signal("property::mwfact")
capi.tag.add_signal("property::useless_gap")
capi.tag.add_signal("property::master_fill_policy")
capi.tag.add_signal("property::ncol")
capi.tag.add_signal("property::nmaster")
capi.tag.add_signal("property::windowfact")
capi.tag.add_signal("property::screen")
capi.tag.add_signal("property::index")
capi.tag.add_signal("property::urgent")
capi.tag.add_signal("property::urgent_count")

capi.screen.add_signal("tag::history::update")
capi.screen.connect_signal("tag::history::update", tag.history.update)

function tag.mt:__call(...)
    return tag.new(...)
end

return setmetatable(tag, tag.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
