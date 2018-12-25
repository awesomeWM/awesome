local args = ...


-- Configuration file for ldoc

project='awesome'
title='awesome API documentation'
description='API documentation for awesome, a highly configurable X window manager (version @AWESOME_VERSION@).'

-- More on it: https://github.com/stevedonovan/LDoc/blob/master/doc/doc.md#markdown-support
format='discount'
dir='../doc'

-- Make the docs prettier
pretty='lua'
style=true
template=true
backtick_references=true
merge=true
use_markdown_titles=true
wrap=true
full_description = [[
Welcome to the documentation for the awesome window manager. Below you find an
overview of the individual parts which links to the full documentation.

If you are a new user, you may want to read @{07-my-first-awesome.md} to get
started. In @{05-awesomerc.md}, the default configuration is explained.

If you already used awesome in the past, @{89-NEWS.md} and @{17-porting-tips.md}
should be useful for you.
]]
topics={
    '00-authors.md',
    '01-readme.md',
    '02-contributing.md',
    '03-declarative-layout.md',
    '04-new-widgets.md',
    '05-awesomerc.md',
    '06-appearance.md',
    '07-my-first-awesome.md',
    '16-using-cairo.md',
    '17-porting-tips.md',
    '90-FAQ.md',
    '89-NEWS.md',
}

-- Setup @client to be an alias for "@tparam client.object"
tparam_alias('client', 'client.object')
tparam_alias('tag', 'tag')
-- Should be default, but is not.  Sets up "@tab" => "@tparam table".
tparam_alias('tab', 'table')
tparam_alias('screen', 'screen')
tparam_alias('screen_or_idx', 'screen|int')

-- The first stereotype are the constructors.
new_type("constructorfct", "Constructors", false, "Parameters")
-- Hack to get the functions on top of the signals and properties
new_type("function", "Functions", false, "Parameters")
-- For "classes", use an explicit type for static functions. This allows
-- @function and its implicit cousin to be banned in the CI.
new_type("staticfct", "Static module functions", false, "Parameters")
-- Documentation for objects properties
new_type("property", "Object properties", false, "Type constraints")
-- Documentation for objects deprecated properties
new_type("deprecatedproperty", "Deprecated object properties", false, "Type constraints")
-- Use a custom type for the methods to bypass the faulty ldoc built-in detection.
-- (yes, the space after Methods *is* on purpose to avoid clashing with ldoc
--  internal "methods" concept)
new_type("method", "Object methods ", false, "Parameters")
-- New type for signals
new_type("signal", "Signals", false, "Arguments")
-- New type for signals connections
new_type("signalhandler", "Request handlers", false, "Arguments")
-- Allow objects to define a set of beautiful properties affecting them
new_type("beautiful", "Theme variables", false, "Type constraints")
-- Put deprecated methods in their own section
new_type("deprecated", "Deprecated functions", false, "Parameters")
-- For the legacy stateless layout related functions
new_type("legacylayout", "Layout related functions", false, "Parameters")
-- Have a category for the client layouts
new_type("clientlayout", "Client layouts", false, "Parameters")
-- Source functions for the taglist/tasklist/layoutlist
new_type("sourcefunction", "List source functions", false)
-- Document some callback prototypes
new_type("callback", "Callback functions prototype", false, "Parameters")
-- gears.matcher / awful.rules sources
new_type("rulesources", "Rule sources", false, "Parameters")
-- gears.matcher / awful.rules rule components
new_type("rulecomponent", "Rule components", false, "Type")
-- Filter functions for the taglist/tasklist/layoutlist
new_type("filterfunction", "List filters", false)
-- Extra client properties available only in awful.rules/spawn constructs
new_type("clientruleproperty", "Extra properties available in awful.rules and awful.spawn", false, "Type")

-- More fitting section names
kind_names={topic='Documentation', module='Libraries', script='Sample files'}

-- Sort modules alphabetically
sort_modules=true

-- Add more project level (left side index) types.
new_type("coreclassmod", "Core_components"  , true)
new_type("widgetmod"   , "Widgets"   , true)
new_type("containermod", "Widget_containers", true)
new_type("layoutmod"   , "Widget_layouts"   , true)
new_type("popupmod"    , "Popups_and_bars"  , true)
new_type("utillib"     , "Utility_libraries", true)
new_type("themelib"    , "Theme_related_libraries", true)

file = {
    -- C parts of libraries
    '../dbus.c',
    '../luaa.c',
    '../mouse.c',
    '../mousegrabber.c',
    '../root.c',
    '../selection.c',
    '../spawn.c',
    '../xkb.c',
    '../common/luaobject.c',
    '../objects/client.c',
    '../objects/drawable.c',
    '../objects/screen.c',
    '../objects/tag.c',
    '../objects/window.c',

    -- LUA libraries
    '../lib/',
    -- Auto generated scripts
    '../script_files/',
    exclude = {
        -- exclude these modules, as they do not contain any written
        -- documentation
        '../lib/awful/autofocus.lua',
        '../lib/awful/dbus.lua',
        '../lib/awful/init.lua',
        '../lib/awful/remote.lua',
        '../lib/awful/screen/dpi.lua',
        '../lib/awful/startup_notification.lua',
        '../lib/awful/mouse/drag_to_tag.lua',
        '../lib/gears/init.lua',
        '../lib/wibox/layout/init.lua',
        '../lib/wibox/container/init.lua',
        '../lib/naughty/constants.lua',
        '../lib/naughty/dbus.lua',
        '../lib/beautiful/gtk.lua',

        -- Ignore some parts of the widget library
        '../lib/awful/widget/init.lua',
        '../lib/naughty/layout/init.lua',
        '../lib/naughty/widget/init.lua',
        '../lib/naughty/container/init.lua',
        '../lib/naughty/list/init.lua',
        '../lib/naughty/widget/_default.lua',

        -- Ignore components that provide no value to users and have confusing
        -- names
        '../objects/button.c',
        '../objects/key.c',

        -- Deprecated classes for one years or more don't deserve entries
        -- in the index
        '../lib/awful/widget/graph.lua',
        '../lib/awful/widget/progressbar.lua',
        '../lib/awful/widget/textclock.lua',
        '../lib/awful/wibox.lua',
        '../lib/wibox/layout/constraint.lua',
        '../lib/wibox/layout/margin.lua',
        '../lib/wibox/layout/mirror.lua',
        '../lib/wibox/layout/rotate.lua',
        '../lib/wibox/layout/scroll.lua',
        '../lib/wibox/widget/background.lua',
    }
}

-- Wrap the module name for the CSS highlight.
local function wrap_modname(str, item)
    if (not item.module) or str:sub(1, #item.module.name+1) ~= item.module.name.."." then return str end

    return "<span class='function_modname'>"..item.module.name..".</span>"
        .. str:sub(#item.module.name+2, 9999)
end

local named_args = {
    [ "(args)"       ] = true,
    [ "([args])"     ] = true,
    [ "([args=nil])" ] = true,
    [ "([args={}])"  ] = true
}

-- Wrap the arguments for the CSS highlight.
local function wrap_args(item)
    if not item.args then return "" end

    -- Display named args with `{}` and ordered args with `()`
    if named_args[item.args] then
        return "<span class='function_named_args'><b>{</b>[args]<b>}</b></span>"
    end

    local new_args = item.args:sub(2, item.args:len()-1)

    return "<span class='function_args'> <b>(</b>"..new_args.."<b>)</b></span>"
end


-- Mimics the ldoc built-in method style, but better.
--
-- This custom renderer exists because using ldoc built-in method detection
-- turned out to be too unreliable and upstream is dead.
local function render_methods(item)
    local ret = item.name

    -- Some methods will have it depending on the weather. Most wont.
    if not ret:find(":") then
        ret = ":"..ret
    end

    return ret .. " " .. wrap_args(item)
end

-- Parse the magic parameters to type concention in something the template eats.
local function sanitize_type(item, ldoc)

    for parm in ldoc.modules.iter(item.params) do
        local t = item:type_of_param(parm)

        -- Remove the value.
        t = t:gsub("(\\[[^\\]]])","")
        t = t:gsub("?","")

        -- Add " or " between alternatives
        t = t:gsub("|"," <i>or</i> ")

        -- Fallback.
        t = t == "" and parm or t
        t = t == "" and "N/A" or t

        item.display_type = "<span class='summary_type'>"..t.."</span>"

        -- There is no value in repeating the type a second time.
        if item.params.map[parm] == "" then
            item.hide_params = true
        end

        if t ~= "N/A" then return end
    end

    -- It has to be set, otherwise the table will have different col count.
    item.display_type = "<span class='summary_type'>N/A</span>"
end
local no_prefix = {
    property = true, signal = true, clientruleproperty = true,
    deprecatedproperty = true,
}

-- These modules merge the doc of their `awful` siblings.
local coreclassmap = {
    tag    = "tag<span class='listplusign'> and awful.tag</span>",
    screen = "screen<span class='listplusign'> and awful.screen</span>",
    client = "client<span class='listplusign'> and awful.client</span>",
    mouse  = "mouse<span class='listplusign'> and awful.mouse</span>",
}

-- Add the full module name in front.
local add_mod = {
    ["function"]   = true,
    constructorfct = true,
    staticfct      = true,
    deprecated     = true,
    field          = true,
}

-- Add the arguments.
local add_args = {
    constructorfct = true,
    staticfct      = true,
}

-- Add a type column to the summary and type field in the description.
local display_type = {
    property           = true,
    beautiful          = true,
    field              = true,
    deprecatedproperty = true,
}

-- Show return values.
local show_return = {
    ["function"]   = true,
    constructorfct = true,
    staticfct      = true,
    method         = true,
    deprecated     = true,
}

custom_display_name_handler = function(item, default_handler)
    local ret = default_handler(item)

    -- Edit the input so the template is notified.
    if display_type[item.type] then
        -- Punch a hole in the sandbox and inject the `ldoc` object.
        item.sanitize_type = sanitize_type
    end

    -- LDoc hardcode the "Returns" section for "function" only, fix that.
    if show_return[item.type] and item.tags["return"] then
        item.ret = item.tags["return"]
        item:build_return_groups()
    end

    -- Remove the "namespace" from the signals and properties
    if no_prefix[item.type] then
        local name = item.name:match("%.([^.]+)$")
        return name ~= "" and name or item.name
    end

    -- Handle the left sidebar modules.
    if item.type == "coreclassmod" and coreclassmap[item.name] then
        return coreclassmap[item.name]
    end

    -- Undocumented API to make the libraries and classmod "function" section
    -- more consistent. Right now some have their full awful.foo.bar while other
    -- have "just" `bar`. Given we use constructors from metatables, we have no
    -- choice but to use the full function name. It also makes copy/paste easier.
    if add_mod[item.type] then
        if (not ret:find(".", 1, true)) and (not ret:find(":", 1, true))  then
            ret = item.module.name .. "." .. ret
        elseif item.type == "field" and ret:sub(1, #item.module.name) ~= item.module.name then
            ret = item.module.name .. "." .. ret
        end
    end

    if item.type == "deprecated" or item.type == "deprecatedproperty" then
        return ret .. "<i class=\"deprecated_label\"> [deprecated]</i>"
    end

    if item.type == "method" then
        ret = render_methods(item)
    end


    -- Get rid of the "module:" in front of method names. It is either wrong or
    -- just redundant.
    ret = ret:gsub("([^:]*)(:[^:])","%2")

    -- Undocumented API to get rid of `module.rounded_rect` rather than
    -- `gears.shape.rounded_rect`
    if ret:sub(1, 7) == "module." and module then
        return ret:gsub("^module", item.module.name)
    end

    -- It isn't there by default.
    if add_args[item.type] then
        ret = ret .. " " .. wrap_args(item)
    end

    -- Beautify.
    ret = wrap_modname(ret, item)

    return ret
end

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
