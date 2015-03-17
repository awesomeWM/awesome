---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local util = require("awful.util")
local tostring = tostring
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

--- Useful functions for tag manipulation.
-- awful.tag
local tag = { mt = {} }

-- Private data
local data = {}
data.history = {}
data.tags = setmetatable({}, { __mode = 'k' })

-- History functions
tag.history = {}
tag.history.limit = 20

--- Move a tag to an absolute position in the screen[]:tags() table.
-- @param new_index Integer absolute position in the table to insert.
-- @param target_tag The tag that should be moved. If null, the currently
-- selected tag is used.
function tag.move(new_index, target_tag)
    local target_tag = target_tag or tag.selected()
    local scr = tag.getscreen(target_tag)
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
        tag.setscreen(tmp_tag, scr)
        tag.setproperty(tmp_tag, "index", i)
    end
end

--- Add a tag.
-- @param name The tag name, a string
-- @param props The tags properties, a table
-- @return The created tag
function tag.add(name, props)
    local properties = props or {}

    -- Be sure to set the screen before the tag is activated to avoid function
    -- connected to property::activated to be called without a valid tag.
    -- set properies cannot be used as this has to be set before the first signal
    -- is sent
    properties.screen = properties.screen or capi.mouse.screen

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
    local screen = screen or 1
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
-- @param screen The screen number to look for a tag on. [mouse.screen]
-- @param invalids A table of tags we consider unacceptable. [selectedlist(scr)]
function tag.find_fallback(screen, invalids)
    local scr = screen or capi.mouse.screen
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
    local target_tag = target_tag or tag.selected()
    if target_tag == nil or target_tag.activated == false then return end

    local target_scr = tag.getscreen(target_tag)
    local tags       = tag.gettags(target_scr)
    local idx        = tag.getidx(target_tag)
    local ntags      = #tags

    -- We can't use the target tag as a fallback.
    local fallback_tag = fallback_tag
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

        -- If a client has only this tag, or stickied clients with
        -- nowhere to go, abort.
        if (not c.sticky and #c:tags() == 1) or
                                    (c.sticky and fallback_tag == nil) then
            return
        -- If a client has multiple tags, then do not move it to fallback
        elseif #c:tags() < 2 then
            c:tags({fallback_tag})
        end
    end

    -- delete the tag
    data.tags[target_tag].screen = nil
    target_tag.activated = false

    -- Update all indexes
    for i=idx+1,#tags do
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
    local s = obj.index
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
-- @param screen The screen number.
-- @param idx Index in history. Defaults to "previous" which is a special index
-- toggling between last two selected sets of tags. Number (eg 1) will go back
-- to the given index in history.
function tag.history.restore(screen, idx)
    local s = screen or capi.mouse.screen
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
end

--- Get a list of all tags on a screen
-- @param s Screen number
-- @return A table with all available tags
function tag.gettags(s)
    local tags = {}
    for i, t in ipairs(root.tags()) do
        if tag.getscreen(t) == s then
            table.insert(tags, t)
        end
    end

    table.sort(tags, function(a, b)
        return (tag.getproperty(a, "index") or 9999) < (tag.getproperty(b, "index") or 9999) 
    end)
    return tags
end

--- Set a tag's screen
-- @param t tag object
-- @param s Screen number
function tag.setscreen(t, s)
    local s = s or capi.mouse.screen
    local sel = tag.selected
    local old_screen = tag.getproperty(t, "screen")
    if s == old_screen then return end

    -- Keeping the old index make very little sense when changing screen
    tag.setproperty(t, "index", nil)

    -- Change the screen
    tag.setproperty(t, "screen", s)

    -- Make sure the client's screen matches its tags
    for k,c in ipairs(t:clients()) do
        c.screen = s --Move all clients
        c:tags({t})
    end

    -- Update all indexes
    for _,screen in ipairs {old_screen,s} do
        for i,t in ipairs(tag.gettags(screen)) do
            tag.setproperty(t, "index", i)
        end
    end

    -- Restore the old screen history if the tag was selected
    if sel then
        tag.history.restore(old_screen,1)
    end
end

--- Get a tag's screen
-- @param t tag object
-- @return Screen number
function tag.getscreen(t)
    return tag.getproperty(t, "screen")
end

--- Return a table with all visible tags
-- @param s Screen number.
-- @return A table with all selected tags.
function tag.selectedlist(s)
    local screen = s or capi.mouse.screen
    local tags = tag.gettags(screen)
    local vtags = {}
    for i, t in pairs(tags) do
        if t.selected then
            vtags[#vtags + 1] = t
        end
    end
    return vtags
end

--- Return only the first visible tag.
-- @param s Screen number.
function tag.selected(s)
    return tag.selectedlist(s)[1]
end

--- Set master width factor.
-- @param mwfact Master width factor.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.setmwfact(mwfact, t)
    local t = t or tag.selected()
    if mwfact >= 0 and mwfact <= 1 then
        tag.setproperty(t, "mwfact", mwfact)
    end
end

--- Increase master width factor.
-- @param add Value to add to master width factor.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.incmwfact(add, t)
    tag.setmwfact(tag.getmwfact(t) + add, t)
end

--- Get master width factor.
-- @param t Optional tag.
function tag.getmwfact(t)
    local t = t or tag.selected()
    return tag.getproperty(t, "mwfact") or 0.5
end

--- Set the number of master windows.
-- @param nmaster The number of master windows.
-- @param t Optional tag.
function tag.setnmaster(nmaster, t)
    local t = t or tag.selected()
    if nmaster >= 0 then
        tag.setproperty(t, "nmaster", nmaster)
    end
end

--- Get the number of master windows.
-- @param t Optional tag.
function tag.getnmaster(t)
    local t = t or tag.selected()
    return tag.getproperty(t, "nmaster") or 1
end

--- Increase the number of master windows.
-- @param add Value to add to number of master windows.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.incnmaster(add, t)
    tag.setnmaster(tag.getnmaster(t) + add, t)
end


--- Set the tag icon
-- @param icon the icon to set, either path or image object
-- @param _tag the tag
function tag.seticon(icon, _tag)
    local _tag = _tag or tag.selected()
    tag.setproperty(_tag, "icon", icon)
end

--- Get the tag icon
-- @param _tag the tag
function tag.geticon(_tag)
    local _tag = _tag or tag.selected()
    return tag.getproperty(_tag, "icon")
end

--- Set number of column windows.
-- @param ncol The number of column.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.setncol(ncol, t)
    local t = t or tag.selected()
    if ncol >= 1 then
        tag.setproperty(t, "ncol", ncol)
    end
end

--- Get number of column windows.
-- @param t Optional tag.
function tag.getncol(t)
    local t = t or tag.selected()
    return tag.getproperty(t, "ncol") or 1
end

--- Increase number of column windows.
-- @param add Value to add to number of column windows.
-- @param t The tag to modify, if null tag.selected() is used.
function tag.incncol(add, t)
    tag.setncol(tag.getncol(t) + add, t)
end

--- View no tag.
-- @param Optional screen number.
function tag.viewnone(screen)
    local tags = tag.gettags(screen or capi.mouse.screen)
    for i, t in pairs(tags) do
        t.selected = false
    end
end

--- View a tag by its taglist index.
-- @param i The relative index to see.
-- @param screen Optional screen number.
function tag.viewidx(i, screen)
    local screen = screen or capi.mouse.screen
    local tags = tag.gettags(screen)
    local showntags = {}
    for k, t in ipairs(tags) do
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
    capi.screen[screen]:emit_signal("tag::history::update")
end

--- Get a tag's index in the gettags() table.
-- @param query_tag The tag object to find. [selected()]
-- @return The index of the tag, nil if the tag is not found.
function tag.getidx(query_tag)
    local query_tag = query_tag or tag.selected()
    if query_tag == nil then return end

    for i, t in ipairs(tag.gettags(tag.getscreen(query_tag))) do
        if t == query_tag then
            return i
        end
    end
end

--- View next tag. This is the same as tag.viewidx(1).
-- @param screen The screen number.
function tag.viewnext(screen)
    return tag.viewidx(1, screen)
end

--- View previous tag. This is the same a tag.viewidx(-1).
-- @param screen The screen number.
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
-- @param screen Optional screen number of the tags.
function tag.viewmore(tags, screen)
    local screen = screen or capi.mouse.screen
    local screen_tags = tag.gettags(screen)
    for _, _tag in ipairs(screen_tags) do
        if not util.table.hasitem(tags, _tag) then
            _tag.selected = false
        end
    end
    for _, _tag in ipairs(tags) do
        _tag.selected = true
    end
    capi.screen[screen]:emit_signal("tag::history::update")
end

--- Toggle selection of a tag
-- @param tag Tag to be toggled
function tag.viewtoggle(t)
    t.selected = not t.selected
    capi.screen[tag.getscreen(t)]:emit_signal("tag::history::update")
end

--- Get tag data table.
-- @param tag The Tag.
-- @return The data table.
function tag.getdata(_tag)
    return data.tags[_tag]
end

--- Get a tag property.
-- @param _tag The tag.
-- @param prop The property name.
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
function tag.setproperty(_tag, prop, value)
    if not data.tags[_tag] then
        data.tags[_tag] = {}
    end
    data.tags[_tag][prop] = value
    _tag:emit_signal("property::" .. prop)
end

--- Tag a client with the set of current tags.
-- @param c The client to tag.
function tag.withcurrent(c)
    local tags = {}
    for k, t in ipairs(c:tags()) do
        if tag.getscreen(t) == c.screen then
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
    capi.tag.connect_signal(sig, function(_tag, ...)
        if tag.getscreen(_tag) == screen then
            func(_tag)
        end
    end)
end

--- Add a signal to all attached tag and all tag that will be attached in the
-- future. When a tag is detach from the screen, its signal is removed.
-- @param screen The screen concerned, or all if nil.
function tag.attached_connect_signal(screen, ...)
    if screen then
        attached_connect_signal_screen(screen, ...)
    else
        capi.tag.connect_signal(...)
    end
end

-- Register standards signals
capi.client.connect_signal("manage", function(c, startup)
    -- If we are not managing this application at startup,
    -- move it to the screen where the mouse is.
    -- We only do it for "normal" windows (i.e. no dock, etc).
    if not startup and c.type ~= "desktop" and c.type ~= "dock" then
        if c.transient_for then
            c.screen = c.transient_for.screen
            if not c.sticky then
                c:tags(c.transient_for:tags())
            end
        else
            c.screen = capi.mouse.screen
        end
    end
    c:connect_signal("property::screen", tag.withcurrent)
end)

capi.client.connect_signal("manage", tag.withcurrent)
capi.tag.connect_signal("request::select", tag.viewonly)

capi.tag.add_signal("property::hide")
capi.tag.add_signal("property::icon")
capi.tag.add_signal("property::icon_only")
capi.tag.add_signal("property::layout")
capi.tag.add_signal("property::mwfact")
capi.tag.add_signal("property::ncol")
capi.tag.add_signal("property::nmaster")
capi.tag.add_signal("property::windowfact")
capi.tag.add_signal("property::screen")
capi.tag.add_signal("property::index")

for s = 1, capi.screen.count() do
    capi.screen[s]:add_signal("tag::history::update")
    capi.screen[s]:connect_signal("tag::history::update", tag.history.update)
end

function tag.mt:__call(...)
    return tag.new(...)
end

return setmetatable(tag, tag.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
