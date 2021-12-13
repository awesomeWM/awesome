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
Welcome to the documentation for the Awesome window manager. Below you find an
overview of the individual parts which links to the full documentation.

If you are a new user, you may want to read @{07-my-first-awesome.md} to get
started. In @{05-awesomerc.md}, the default configuration is explained.

If you already used awesome in the past, @{89-NEWS.md} and @{17-porting-tips.md}
should be useful for you.

### Default configuration components name:

<center><img src="images/AUTOGEN_awful_popup_defaultconfig.svg"></center>

### Guides

<div class="index_guides">
    <div>
        @{07-my-first-awesome.md|Getting started}
        @{90-FAQ.md|FAQ}
        @{01-readme.md|Read Me}
        @{89-NEWS.md|NEWS}
    </div>
    <div>
        @{03-declarative-layout.md|The widget system}
        @{09-options.md|Startup options}
        @{05-awesomerc.md|The default rc.lua}
        @{08-client-layout-system.md|Window management}
    </div>
</div>

## Major libraries

AwesomeWM ship multiple libraries. Here is an overview of the purpose and scope
of those libraries.

<table class='widget_list' border=1>
  <tr style='font-weight: bold;'>
   <th align='center'>Library</th>
   <th align='center'>Description</th>
  </tr>
  <tr><td>`gears`</td><td>Utilities such as color parsing and objects</td></tr>
  <tr><td>`wibox`</td><td>Awesome own generic widget framework</td></tr>
  <tr><td>`awful`</td><td>Everything related to window managment</td></tr>
  <tr><td>`awful.widget`</td><td>Window management related widgets</td></tr>
  <tr><td>`awful.layout`</td><td>The default stateless client tiling module.</td></tr>
  <tr><td>`ruled`</td><td>Define declarative rules on various events</td></tr>
  <tr><td>`naughty`</td><td>Notifications</td></tr>
  <tr><td>`menubar`</td><td>XDG (application) menu implementation</td></tr>
  <tr><td>`beautiful`</td><td>Awesome theme module</td></tr>
</table>

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
    '08-client-layout-system.md',
    '09-options.md',
    '10-building-and-testing.md',
    '16-using-cairo.md',
    '17-porting-tips.md',
    '90-FAQ.md',
    '89-NEWS.md',
}

-- The first stereotype are the constructors.
new_type("constructorfct", "Constructors", false, "Parameters")
new_type("constructorfct2", "ldoc_skip", false, "Parameters")
-- Hack to get the functions on top of the signals and properties
new_type("function", "Functions", false, "Parameters")
-- For "classes", use an explicit type for static functions. This allows
-- @function and its implicit cousin to be banned in the CI.
new_type("staticfct", "Static module functions", false, "Parameters")
-- Documentation for objects properties
new_type("property", "Object properties", false, "Type constraints")
-- Documentation for objects deprecated properties
new_type("deprecatedproperty", "Deprecated object properties", false, "Type constraints")
-- Documentation for objects deprecated methods
new_type("deprecatedmethod", "Deprecated object methods", false, "Parameters")
-- Use a custom type for the methods to bypass the faulty ldoc built-in detection.
-- (yes, the space after Methods *is* on purpose to avoid clashing with ldoc
--  internal "methods" concept)
new_type("method", "Object methods ", false, "Parameters")
-- New type for signals
new_type("signal", "Signals", false, "Arguments")
-- Deprecated signals.
new_type("deprecatedsignal", "Deprecated signals", false, "Arguments")
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
new_type("clientruleproperty", "Extra properties available in the rules", false, "Type")
-- Extra *matching* properties for rules.
new_type("matchingproperty", "Extra matching properties used in rules", false, "Type")

-- Simulate the default "params" parser format, except the optional "[]" section
-- needs a space.
local function parse_custom_tags(text, params)
    text = text:gmatch("[ ]*(.*)$")()
    local raw_extra = ""

    if text:sub(1,1) == '[' then
        local count = 1

        -- Find the matching ']'
        for i=2, text:len() do
            local char = text:sub(i,i)

            if char == '[' then
                count = count + 1
            elseif char == ']' then
                count = count - 1
            end

            raw_extra = raw_extra..char

            if count == 0 then
                text = text:sub(i+2)
                break
            end
        end
    end

    -- Split the remaining text into words.
    local words, values, description = {}, {}, {}

    for word in text:gmatch("[^ \n\r]*") do
        if word ~= "" then
            words[#words+1] = word
        end
    end

    for idx, word in ipairs(words) do
        if idx <= #params then
            local name = params[idx].name
            values[name] = {
                name  = name,
                title = params[idx].title or name,
                value = word
            }
        else
            description[#description+1] = word
            values.description = values.description and
                values.description.." "..word or
                word
        end
    end

    return values
end

-- Mimic the template classes.
local function default_format_callback(self, params, _, md)
    local ret = ""

    if self.table then
        -- All columns are mandatory
        for _, p in ipairs(self.params) do
            local content = params[p.name] and params[p.name].value or ""
            ret = ret.."<td>"..(p.markdown and md("`"..content.."`") or content).."</td>"
        end
        return ret .. "<td>"..md(params.description).."</td>"
    else
        if params.name then
            ret = '<span class="parameter">'..
                md("`"..params.name.value.."`")..
            "</span> "
        end

        if params.type then
            ret = ret .. '<span class="types">'..
                md("`"..params.type.value.."`")..
            "</span>"
        end

        return ret.." "..md(params.description)
    end
end

-- Generate a format function.
local function default_format(self, callback)
    return function(raw, item, md)
        return (callback or default_format_callback)(
            self, parse_custom_tags(raw, self.params or {}), item, md
        )
    end
end

local named_tags, item_id = {}, 1

-- Add a new @something which can be used in any types.
-- @tparam table args
-- @tparam string args.name The name.
-- @tparam string args.hidden Show in the doc or for internal use only.
-- @tparam table args.table Show the items in a table rather than a list. The
--  content is the list of header names.
-- @tparam table args.params The parameters (table with name, tilte, format).
-- @tparam boolean[opt=true] args.auto_subtags Create the usage and tparams subtags.
local add_custom_tag
add_custom_tag = function(args)
    local name = args.name

    args.name, args[1] = nil, name

    custom_tags = custom_tags or {}

    local f = args.format

    args.format = default_format(args, f)

    custom_tags[#custom_tags+1] = args
    named_tags[name] = args

    -- Auto create @name_tparams and @name_usage for each custom tags.
    if args.auto_subtags ~= false then
        add_custom_tag {
            name         = name.."tparam",
            auto_params  = true,
            parent       = args,
            auto_subtags = false,
            params       = {
                { name   = "type" },
                { name   = "name" },
            }
        }
        add_custom_tag {
            name         = name.."usage",
            auto_usage   = true,
            parent       = args,
            auto_subtags = false,
        }
    end
end

-- When a type will request a permission.
-- @emits class signal Message[...]
add_custom_tag {
    name   = "emits",
    title  = "Emit signals",
    hidden = false,
    params = {
        {
            name = "name"
        }
    }
}

-- Avoid repetitive boilerplate code for property signals.
-- Add true if the signal has the value or false if it doesn't.
-- @propemits true/false true/false description[...]
add_custom_tag {
    name   = "propemits",
    title  = "Emit signals",
    hidden = false,
    params = {{name = "new_value"}, {name = "old_value"}},
    format = function(self, params, item, md)
        -- Add an automatic fallback description.
        local description = params.description ~= "" and params.description or
            "When the `"..item.name.."` value changes."
        local new_value = params.new_value.value == "true"
        local old_value = params.old_value.value == "true"

        -- Add the sub-tags.
        local subs = {}
        item.auto_params["propemitstparam_override"] = subs

        -- The first argument is always the object which changes.
        subs[1] = item.module.name.." self ".." The object which changed ("..
            "useful when connecting many object to the same callback)."

        -- Most signals also have the new value.
        if new_value then
            local type = item.params[1] or "unknown"
            subs[2] = type.." ".."new_value The new value affected to the property."
        end

        -- Some also have the old value.
        if old_value then
            local type = item.params[1] or "unknown"
            subs[3] = type.." ".."old_value The property's old value."
        end

        local new_params = {
            name = { name  = item.name, value = "property::"..item.name },
            description = description
        }

        return default_format_callback(self, new_params, item, md)
    end
}

-- List the beautiful variables used by the method or property fallbacks.
-- @usebeautiful beautiful.varname usage[...]
add_custom_tag {
    name   = "usebeautiful",
    title  = "Consumed theme variables",
    hidden = false,
    table  = {
        "Variable", "Usage"
    },
    params = {
        {
            name     = "name",
            markdown = true,
        }
    }
}

-- For all properties which have a standard `@beautiful` variable for them
-- @propbeautiful fallback1 fallback2 fallback3 fallback4
add_custom_tag {
    name   = "propbeautiful",
    title  = "Consumed theme variables",
    params = {
        { name = "fallback1" },
        { name = "fallback2" },
        { name = "fallback3" },
        { name = "fallback4" },
    },
    table  = {
        "Variable", "Usage"
    },
    format = function(self, p, item, md)
        local modname = item.module.name:gmatch("[^.]+$")()
        local last = "beautiful."..(modname.."_"..item.name):gsub("[.]", "_")
        local ret = "<td>"..md("`"..last.."`").."</td><td>Fallback when "..md("`"..item.name.."`")..
            " isn't set.</td>"

        for _, fallback in ipairs({p.fallback1, p.fallback2, p.fallback3, p.fallback4 }) do
            ret = ret .. "</tr><tr>"..
                "<td>"..md("`"..fallback.value.."`").."</td><td>Fallback when "..md("`"..last.."`")..
            " isn't set.</td>"
            last = fallback.value
        end

        return ret
    end
}

-- Define the base class where a method/property is implemented.
-- @baseclass my_module.my_submodule.my_baseclass
add_custom_tag {
    name   = "baseclass",
    hidden = true
}

-- Define which interface a method or property implements.
-- @interface container
add_custom_tag {
    name   = "interface",
    hidden = true
}

-- Define when a signal is only emitted on a class rather than on objects.
add_custom_tag {
    name   = "classsignal",
    hidden = true,
}

-- Specify when this an item was deprecated.
-- @deprecatedin 4.4 Optional message.
add_custom_tag {
    name   = "deprecatedin",
    hidden = true,
    params = {
        { name = "api_level" },
    },
}

-- Specify when this an item was deprecated because it was renamed.
-- @renamedin 4.4 new_name Optional message.
add_custom_tag {
    name   = "renamedin",
    hidden = true,
    params = {
        { name = "api_level" },
        { name = "new_name"  },
    },
}

-- Specify when this an item was deprecated because it was moved.
-- @movedin 4.4 new_class new_name Optional message.
add_custom_tag {
    name   = "movedin",
    hidden = true,
    params = {
        { name = "api_level" },
        { name = "new_class" },
        { name = "new_name"  },
    },
}

-- Specify when an item was added.
-- @introducedin 4.4
add_custom_tag {
    name   = "introducedin",
    hidden = true,
    params = {
        { name = "api_level" },
    },
}

add_custom_tag {
    name   = "request",
    title  = "Requested actions or permissions",
    params = {
        { name = "class"   },
        { name = "type"    },
        { name = "context" },
        { name = "default" },
    },
    table  = {
        "Class", "Permission", "Context", "Default", "Description"
    },
}

-- Define the supermodule class.
-- This tag should be used at the module level. All properties from the
-- supermodule will be recursively added to the module by our ldoc template.
-- @supermodule supermodule
add_custom_tag {
    name = "supermodule",
    hidden = true,
    auto_subtags = false
}

-- Mark the item as hidden.
-- This tag should be used to hide items from the documentation.
-- @hidden
add_custom_tag {
    name = "hidden",
    hidden = true,
    auto_subtags = false
}

-- Mark the item as readonly.
-- This tag should be used to mark readonly properties.
-- @readonly
add_custom_tag {
    name = "readonly",
    hidden = true,
    auto_subtags = false
}
-- More fitting section names
kind_names={topic='Documentation', module='Libraries', script='Sample files'}

-- Sort modules alphabetically
sort_modules=true

-- Add more project level (left side index) types.
new_type("coreclassmod", "Core_components"  , true)
new_type("inputmodule" , "Input_handling"   , true)
new_type("ruleslib"    , "Declarative_rules", true)
new_type("widgetmod"   , "Widgets"          , true)
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
        '../lib/awful/client/shape.lua',
        '../lib/awful/dbus.lua',
        '../lib/awful/_compat.lua',
        '../lib/awful/init.lua',
        '../lib/awful/remote.lua',
        '../lib/awful/screen/dpi.lua',
        '../lib/awful/startup_notification.lua',
        '../lib/awful/mouse/drag_to_tag.lua',
        '../lib/awful/permissions/_common.lua',
        '../lib/awful/client/urgent.lua',
        '../lib/gears/init.lua',
        '../lib/wibox/layout/init.lua',
        '../lib/wibox/container/init.lua',
        '../lib/naughty/constants.lua',
        '../lib/naughty/dbus.lua',
        '../lib/beautiful/gtk.lua',
        '../lib/ruled/init.lua',

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
        '../lib/awful/ewmh.lua',
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

-- Sections which are hidden by default, but visible when clicked.
local summarize = {
    is_deprecated = {index = 1, title = "deprecated"      , count = false},
    emits         = {index = 2, title = "signal"          , count = true },
    propemits     = {index = 3, title = "signal"          , count = true },
    usebeautiful  = {index = 4, title = "theme variable"  , count = true },
    propbeautiful = {index = 5, title = "theme variable"  , count = true },
    request       = {index = 6, title = "permission"      , count = true },
    classsignal   = {index = 7, title = "Class level only", count = false},
    readonly      = {index = 8, title = "read only"       , count = false},
}

local delimiter_for_tag = {
    usebeautiful  = { "table class='widget_list' border=1", "table", "tr", "tr", {"Theme variable", "Usage"}},
    propbeautiful = { "table class='widget_list' border=1", "table", "tr", "tr", {"Theme variable", "Usage"}},
    request       = { "table class='widget_list' border=1", "table", "tr", "tr", {"Class", "Permission", "Context", "Default", "Description"}},
}

-- Use the first word of the subtag content to map it to its tag.
local function sort_subtags(item, tag, subtag, values)
    local ret = {}

    for _, value in ipairs(values) do
        local parsed = parse_custom_tags(value, {{name = "maps_to"}})
        ret[parsed.maps_to.value] = ret[parsed.maps_to.value] or {}
        ret[parsed.maps_to.value][#ret[parsed.maps_to.value]+1] = parsed.description
    end

    return ret
end

-- Gather a summary of the tags hidden by default .
local function generate_summary(item)
    local tgs = {}

    for k, v in pairs(summarize) do
        tgs[v.index] = {title=v.title, count=0, showcount=v.count}
    end

    for tag, value in pairs(item.tags) do
        if summarize[tag] then
            tgs[summarize[tag].index].count = #value
        end
    end

    local ret, has_show_more = {}, false

    for k, v in ipairs(tgs) do
        if v.count > 0 then

            if v.count > 1 then
                v.title = v.title .. "s"
            end

            ret[#ret+1] = v
            has_show_more = v.showcount or has_show_more
        end
    end

    item.extra_summary = #ret > 0 and ret or nil
    item.has_show_more = has_show_more
end

-- We have custom types, sub-types and different rendering.
--
-- To avoid added too much business logic in the template, handle this now.
-- Note that this works because the name handler is called when doing the table
-- of content, which is before any custom types is used.
local function init_custom_types(item)
    if item.is_init then return end

    generate_summary(item)

    -- Give each item an unique identifier so the JavaScript can locate them.
    item.uid, item_id = item_id, item_id + 1

    item.delims, item.auto_usage, item.auto_params = {}, {}, {}

    local to_rm = {}

    for tag, values in pairs(item.tags) do
        -- Remove the sub-tags so they don't get rendered as top level ones.
        if named_tags[tag] and named_tags[tag].auto_usage then
            item.auto_usage[tag] = sort_subtags(
                item, named_tags[tag].parent, named_tags[tag], values
            )
            to_rm[#to_rm+1] = tag
        elseif named_tags[tag] and named_tags[tag].auto_params then
            item.auto_params[tag] = sort_subtags(
                item, named_tags[tag].parent, named_tags[tag], values
            )
            to_rm[#to_rm+1] = tag
        end
    end

    -- Remove from the top-level tag list.
    for _, rm in ipairs(to_rm) do
        item.tags[rm] = nil
    end

    -- Set the item base class.
    if item.tags["baseclass"] then
        item.baseclass = item.tags["baseclass"][1]
    end

    if not item.baseclass and item.module then
        item.baseclass = item.module.name
    end

    -- Some methods and properties can be inherited from parent classes.
    -- in those case, they need the explicit `@baseclass` tag.
    item.inherited = item.baseclass and item.module
        and item.module.name ~= item.baseclass

    function item.get_delim(tag)
        if delimiter_for_tag[tag] then
            return delimiter_for_tag[tag][1],
                   delimiter_for_tag[tag][2],
                   delimiter_for_tag[tag][3],
                   delimiter_for_tag[tag][4],
                   delimiter_for_tag[tag][5]
        else
            return "ul", "ul", "li", "li", nil
        end
    end

    -- Allow the template to fetch the right sub-tags.
    function item.get_auto_params(tag, value)
        -- Makes auto-generated subtags easier to implement.
        if item.auto_params[tag.."tparam_override"] then
            return item.auto_params[tag.."tparam_override"], named_tags[tag.."tparam"]
        end

        if not item.auto_params[tag.."tparam"] then return nil, nil end

        local parsed = parse_custom_tags(value, named_tags[tag].params)

        if parsed.name and item.auto_params[tag.."tparam"][parsed.name.value] then
            return item.auto_params[tag.."tparam"][parsed.name.value], named_tags[tag.."tparam"]
        end

        return nil, nil
    end

    item.is_init = true
end

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

-- Replace the "|" in alternative types by "or".
local function pipe_to_or(s)
    s = s:gsub("|"," <i>or</i> ")

    if s:sub(1,1) == "?" then
        s = s:gsub("?","nil <i>or</i> ")
    end

    return s
end

-- Parse the magic parameters to type in something the template eats.
local function sanitize_type(item, ldoc)

    for parm in ldoc.modules.iter(item.params) do
        local t = item:type_of_param(parm)

        -- Remove the value.
        t = t:gsub("(\\[[^\\]]])","")
        t = t:gsub("?","")

        -- Add " or " between alternatives
        t = pipe_to_or(t)

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


-- Add parentheses when there is alternative return types.
local function generate_return_tuple(item, group, ldoc)
    local tuple = {}

    for r in group:iter() do
        local type = item:return_type(r);

        type = pipe_to_or(type)

        if type ~= nil and type ~= "" then
            tuple[#tuple+1] = type
        end
    end

    if #tuple == 1 then
        return tuple[1]
    else
        local ret = "("
        for i, elem in ldoc.ipairs(tuple) do
            ret = ret..elem

            if i ~= #tuple then
                ret = ret..", "
            end
        end
        ret = ret..")"

        return ret
    end
end

-- Create a return type string.
local function sanitize_return_type(item, ldoc)

    local groups = item.retgroups

    item.display_type = ""

    if groups then
        item.display_type = "<span class='summary_type'> -&gt;&nbsp;"

        for i,group in ldoc.ipairs(groups) do
            item.display_type = item.display_type .. generate_return_tuple(
                item,
                group,
                ldoc
            )

            if i ~= #groups then
                item.display_type = item.display_type  .. " <i>or</i> "
            end
        end

        item.display_type = item.display_type .. "</span>"
    end

    item.compact_signature = true
end

-- Work around the fact that tag/awful.tag client/awful.client pages
-- are merged.
local function compare_module_name(input, module)
    for _, mod in ipairs { module, "awful."..module } do
        if input:sub(1, #mod) == mod then return true end
    end

    return false
end

local no_prefix = {
    property           = true,
    signal             = true,
    clientruleproperty = true,
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
    ["function"]    = true,
    constructorfct  = true,
    constructorfct2 = true,
    staticfct       = true,
    deprecated      = true,
    field           = true,
}

-- Add the arguments.
local add_args = {
    constructorfct  = true,
    constructorfct2 = true,
    staticfct       = true,
}

-- Add a type column to the summary and type field in the description.
local display_type = {
    property           = true,
    beautiful          = true,
    field              = true,
    deprecatedproperty = true,
    clientruleproperty = true,
    matchingproperty   = true,
    rulecomponent      = true,
}

-- Add the `-> ret_type` annotation.
local display_return_type = {
    method           = true,
    deprecatedmethod = true,
    staticfct        = true,
}

-- Show return values.
local show_return = {
    ["function"]     = true,
    constructorfct   = true,
    constructorfct2  = true,
    legacylayout     = true,
    staticfct        = true,
    method           = true,
    deprecated       = true,
    deprecatedmethod = true,
}

-- The different type of deprecation.
local is_deprecated = {
    deprecated         = true,
    deprecatedproperty = true,
    deprecatedmethod   = true,
    deprecatedsignal   = true,
}

custom_display_name_handler = function(item, default_handler)
    if is_deprecated[item.type] then
        item.tags.is_deprecated = {true}
    end

    init_custom_types(item)

    local ret = default_handler(item)

    -- Edit the input so the template is notified.
    if display_type[item.type] then
        -- Punch a hole in the sandbox and inject the `ldoc` object.
        item.sanitize_type = sanitize_type
    elseif display_return_type[item.type] then
        item.sanitize_type = sanitize_return_type
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
        local is_field = item.type == "field" or item.type == "tfield"

        if (not ret:find(".", 1, true)) and (not ret:find(":", 1, true))  then
            ret = item.module.name .. "." .. ret
        elseif is_field and not compare_module_name(ret, item.module.name) then
            ret = item.module.name .. "." .. ret
        end
    end

    if item.type:match("method") then
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
