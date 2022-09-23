local args = ...
local ldoc = nil

-- luacheck: globals new_type

-- Configuration file for ldoc

project='awesome' -- luacheck: globals project
title='awesome API documentation'  -- luacheck: globals title
description='API documentation for awesome, a highly configurable X window manager (version @AWESOME_VERSION@).'  -- luacheck: globals description

format='discount'  -- luacheck: globals format
dir='../doc'  -- luacheck: globals dir

-- Make the docs prettier
pretty='lua'  -- luacheck: globals pretty
style=true  -- luacheck: globals style
template=true  -- luacheck: globals template
backtick_references=true  -- luacheck: globals backtick_references
merge=true  -- luacheck: globals merge
use_markdown_titles=true  -- luacheck: globals use_markdown_titles
wrap=true  -- luacheck: globals wrap

 -- luacheck: globals full_description
full_description = [[
Welcome to the documentation for the Awesome window manager. Below you find an
overview of the individual parts which links to the full documentation.

If you are a new user, you may want to read @{07-my-first-awesome.md} to get
started. In @{05-awesomerc.md}, the default configuration is explained.

If you already used awesome in the past, @{89-NEWS.md} and @{17-porting-tips.md}
should be useful for you.

### Default configuration components name:

<center>
  <object class="img-object" data="images/AUTOGEN_awful_popup_defaultconfig.svg" alt="" type="image/svg+xml"></object>
</center>

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

-- Rather han copy paste this hundreds of time, unify these special type
-- descriptions.
local type_fallback_description = {
    image = {
        "string Interpreted as a path to an image file." ,
        "string A valid SVG content." ,
        "cairo A cairo image surface: Directly used as-is." ,
        "librsvg A librsvg handle object: Directly used as-is." ,
        "nil Unset the image." ,
    },
    color = {
        "string An hexadecimal color code, such as `\"#ff0000\"` for red." ,
        "string A color name, such as `\"red\"`." ,
        "table A [gradient table](../theme_related_libraries/gears.color.html)." ,
        "cairo.pattern Any valid [Cairo pattern](https://cairographics.org/manual/cairo-cairo-pattern-t.html)." ,
        "cairo.pattern A texture build from an image by [gears.color.create\\_png\\_pattern](../theme_related_libraries/gears.color.html#create_png_pattern)" ,
    },
    shape = {
        "gears.shape Like `gears.shape.circle`",
        "function This can be used for custom shapes or to set parameters of existing shapes.",
    },
    screen = {
        "screen A valid screen object such as retured by `awful.screen.focused()` or `mouse.screen`.",
        "integer A screen global id. Avoid using this since they are unsorted.",
        "string The `\"primary\"` value is also valid.",
    },
    font = {
        "string A Pango [font description](../widgets/wibox.widget.textbox.html#font).",
        "string An [XFT string](https://wiki.archlinux.org/title/X_Logical_Font_Description), such as `\"-*-dejavu sans mono-medium-r-normal--*-80-*-*-*-*-iso10646-1\"`.",

    },
    template = {
        "table A table containing a widget tree definition. WARNING: This is really a table"..
        " and **NOT** a widget object. Use the `widget = come.class.here` to define the "..
        " topmost class rather than construct an instance."
    },
    placement = {
        "function A custom callback to generate *and set* the geometry.",
        "placement Any of the `awful.placement` function or constructs."
    }
}

-- Document the "common" function prototypes. Some common types, such as
-- the client layouts, the shapes and placements are just glorified functions.
local callback_fallback_description = {
    shape = {
        {"functionparam", "cairo.context", "cr", "A [Cairo context](https://cairographics.org/manual/cairo-cairo-t.html)"},
        {"functionparam", "number", "width", "The area width."},
        {"functionparam", "number", "height", "The area height."},
    },
    placement = {
        {"functionreturn", "table", "A table with an `x`, `y`, `width` and `height` keys."},
        {"functionparam", "object", "obj", "Any object with a `geometry` property or method."},
        {"functionparam", "table", "args", "The `placement` arguments. See `awful.placement` for a complete list."},
    },
}

-- Try to use a single name for common types. For example, mixing `color` and
-- `gears.color` (which is *not* a type) is confusing. It also allows the
-- type description to be auto-generated from `type_fallback_description`.
local type_name_linting = {
    ["gears.color"]     = "color",
    ["gears.shape"]     = "shape",
    ["wibox.widget"]    = "widget",
    ["gears.surface"]   = "image",
    ["surface"]         = "image",
    ["awful.placement"] = "placement",
    ["double"]          = "number",
    ["float"]           = "number",
    ["bool"]            = "boolean",
}

local metadata_tags = {
    "propertyunit", "rangestart", "rangestop", "negativeallowed",
}

local databases, named_tags, item_id, is_init = {}, {}, 1, false
local all_theme_vars, delayed_collect = nil, {}

local function collect_item_common(tab, mod, item)
    tab[item.name] = tab[item.name] or {}

    local var = tab[item.name]
    var[mod.name] = var[mod.name] or {}
    var[mod.name][#var[mod.name] + 1] = item
end

--- Add another custom tag **value** to an existing item.
-- @tparam string tag_data The raw/unparsed string for the
--  tag. It will be parsed like any other tags.
local function add_custom_tag_common(item, tag_name, meta_tag, tag_data)
    item.tags = item.tags or {}

    item.tags[tag_name] = item.tags[tag_name] or {}

    item.tags[tag_name][#item.tags[tag_name] + 1] = tag_data

    if meta_tag.collect_callback then
        delayed_collect[#delayed_collect+1] = {item, item.module, tag_name, tag_data}
    end
end

-- Wrap the global `new_type` to allow us to extend what types can do, the
-- same is done below for custom tags.
local function create_type(args)
    new_type(args.name, args.title, args.project_level, args.subfield_title)

    databases[args.name] = databases[args.name] or {}
    databases[args.name].collect = args.collect_callback
    databases[args.name].finish  = args.finish_callback

    return args
end

-- Add the @usebeautiful from the optional/default value.
local function auto_add_usebeautiful(item)
    local vars, var_names = {}, {}

    -- Datamine the default values to autogenerate the @usebeautiful.
    for parm in ldoc.modules.iter(item.params) do
        for p in ldoc.modules.iter(item:subparam(parm)) do
            local def = item:default_of_param(p)

            if def and def ~= true then
                for var in def:gmatch("beautiful[.][a-z_]+") do
                    if not var_names[var] then
                        vars[#vars+1] = var
                    end
                    var_names[var] = true
                end
            end
        end
    end

    for _, var in ipairs(vars) do
        named_tags.usebeautiful:add_to_item(item, var)
    end
end

-- Handle both manually specified callback prototypes and common ones.
local function parse_properties_function_metadata(item, types)
    local raw_input  = item.tags["functionnoparam"] and {} or item.tags["functionparam"]
    local raw_output = item.tags["functionnoreturn"] and {} or item.tags["functionreturn"]
    local fallback, input, output = true, {}, {}

    for _, arg in ipairs(raw_input or {}) do
        input[#input+1] = item.parsed_tags["functionparam"][arg]
        fallback = false
    end

    for _, ret in ipairs(raw_output or {}) do
        output[#output+1] = item.parsed_tags["functionreturn"][ret]
        fallback = false
    end

    local has_in  = #input > 0 or item.tags["functionnoparam"] ~= nil
    local has_out = #output > 0 or item.tags["functionnoreturn"] ~= nil

    if fallback then
        for type in pairs(types) do
            if callback_fallback_description[type] then
                for _, row in ipairs(callback_fallback_description[type]) do
                    local dest = row[1] == "functionparam" and input or output

                    dest[#dest+1] = {
                        description = row[#row],
                        type = row[2] and {value = row[2]},
                        name = row[3] and {value = row[3]},
                    }
                end

                -- Assume the fallback is correct.
                has_out, has_in = true, true
                break
            end
        end
    end

    return {
        input      = input,
        output     = output,
        has_input  = has_in,
        has_output = has_out,
    }
end

-- The first stereotype are the constructors.
create_type  {
    name             = "constructorfct",
    title            = "Constructors",
    project_level    = false,
    subfield_title   = "Parameters",
    collect_callback = auto_add_usebeautiful,
}

-- Some constructors like `awful.key` have both a named parameter
-- syntax and a multiple function parameter syntax.
create_type {
    name           = "constructorfct2",
    title          = "ldoc_skip",
    project_level  = false,
    subfield_title = "Parameters",
}

-- Hack to get the functions on top of the signals and properties
create_type {
    name           = "function",
    title          = "Functions",
    project_level  = false,
    subfield_title = "Parameters",
}

-- For "classes", use an explicit type for static functions. This allows
-- @function and its implicit cousin to be banned in the CI.
create_type {
    name           = "staticfct",
    title          = "Static module functions",
    project_level  = false,
    subfield_title = "Parameters",
}

-- Documentation for objects properties
create_type {
    name             = "property",
    title            = "Object properties",
    project_level    = false,
    subfield_title   = "Type constraints",
    collect_callback = auto_add_usebeautiful,
    finish_callback  = function(item)
        -- All properties need to have a type. Otherwise they don't render
        -- properly. Also, because typed properties are kind of the point of
        -- switching to ldoc in the first place.
        if (not item.params) or not item.params[1] then
            print(
                "WARNING: The ".. item.name .." property from "..item.module.name.." is missing it's type."
            )
            return
        end

        local _, sublist = item:subparam(item.params[1])
        local type = item:type_of_param(item.params[1])

        -- Force people to use @tparam because it is easier to lint and allows
        -- multiple types.
        if (not type) or type == "" then
            print(
                "WARNING: Property ".. item.name .." from "..item.module.name.." either "..
                " doesn't have a type or uses @param instead of @tparam. @tparam is required"..
                " because it allows multi-type and better default value handling."
            )
        end

        if type_name_linting[type] then
            print(
                "WARNING: Property ".. item.name .." from "..item.module.name..
                " type is `"..type.."`, please use `"..type_name_linting[type].."`."
            )
        end

        -- One of the repeated problem we have is the first word of the
        -- description being removed because it is used as the property name.
        -- This "rule" might be stupid, but it prevents it from accidentally
        -- happening again.
        if item.params[1] ~= item.name:match("[.]?("..item.params[1]..")$") then
            print(
                "WARNING: Property ".. item.name .." from "..item.module.name.." @tparam name"..
                " doesn't match the property name. For linting purpose, please fix this."
            )
        end

        local def = item:default_of_param(item.params[1])

        -- Check the default value for obvious mistakes.
        if def then
            -- Detect the blatant missing quote marks for string. This is important
            -- to differentiate variables from string literals.
            if type == "string" and def:sub(1,1) ~= '"' and def:sub(-1) ~= '"' and not def:find(".") then
                print(
                    "WARNING: Property ".. item.name .." from "..item.module.name..
                    " is a string, but the default value is not quoted."
                )
            end

            -- If the default value is `nil`, then the property must be nullable.
            if def == "nil" and not type:find("nil") then
                print(
                    "WARNING: Property ".. item.name .." from "..item.module.name..
                    " default value is `nil`, but the type doesn't allow it"
                )
            end

            if type == "boolean" and (not (def == "true" or def == "false")) and (not def:find(".")) then
                print(
                    "WARNING: Property ".. item.name .." from "..item.module.name..
                    " is a boolean, but is neither `true`, `false` or an alias"
                )
            end

            item:add_metadata("Default value", ("<code>%s</code>"):format(def))
        else
            -- Properties should have a default value. If they don't or if the
            -- default depends on the context, then `opt=nil` should be used to
            -- mute this warning.
            local auto_opt = nil

            -- Extract the default value from other metadata.
            if item.tags["propbeautiful"] and item.tags["propbeautiful"][1] then
                auto_opt = "beautiful."..(item.module.name.."_"..item.name):gsub("[.]", "_")
            elseif item.tags["usebeautiful"] and item.tags["usebeautiful"][1] then
                auto_opt = item.tags["usebeautiful"][1]:match("[^ ]+")
            end

            -- This adds a default value. It works for LDoc 1.4.5, but is a
            -- private API and might break in the future.
            if item.tags["propertydefault"] and item.tags["propertydefault"][1] then
                item:add_metadata(
                    "Default value",
                    ldoc.markup(item.tags["propertydefault"][1])
                )
            elseif auto_opt then
                local mods = item.modifiers[item.parameter]

                if mods then
                    mods[item.params[1]].opt = auto_opt
                end

                item:add_metadata("Default value", ("<code>%s</code>"):format(auto_opt))
            elseif not sublist then
                -- The default could not be determined automatically, it requires
                -- an explicit `opt=<value>`.
                print(
                    "WARNING: Property", item.name .." from "..item.module.name,
                    "doesn't have a default value. Add `[opt=value]` to the @tparam"
                )
            end
        end

        local prop_types, prop_type_names, fallback_callback = {}, {}, false

        -- Type validation.
        do
            -- Handle the case where a property has multiple types.
            local metatypes, table_args = {}, {}
            local prop_has_nil, prop_has_obj = false, false

            -- Make a copy of the table to avoid recursion.
            for _, t in ipairs(item.tags["propertytype"] or {}) do
                metatypes[#metatypes+1] = t
            end

            for t in item:type_of_param(item.params[1]):gmatch('[^|]+') do
                if type_name_linting[t] then
                    print(
                        "WARNING: Property", item.name .." from "..item.module.name..
                        " type is '"..t.."'. Please use `"..type_name_linting[t].."'"
                    )
                end

                if not prop_type_names[t] then
                    prop_types[#prop_types+1] = t
                end
                prop_type_names[t] = true
                fallback_callback = fallback_callback or callback_fallback_description[t]
                prop_has_nil = prop_has_nil or t == "nil"
                prop_has_obj = prop_has_obj or (t:find('.') and not t:sub(1,5) == "gears")
            end

            if item.tags["propbeautiful"]
              and item.tags["propbeautiful"][1]
              and not prop_type_names["nil"] then
                print(
                    "WARNING: Property", item.name .." from "..item.module.name..
                    " uses `@propbeautiful`, yet, doesn't have `nil` as a type."
                )
            end

            if #item.params == 0 then
                print(
                    "WARNING: Property", item.name .." from "..item.module.name..
                    " is missing it's `@tparam` section. Please add it."
                )
            elseif #item.params > 1 then
                for i=2, #item.params do
                    local splitted = {}

                    for part in item.params[i]:gmatch("[^.]+") do
                        splitted[#splitted+1] = part
                    end

                    if #splitted == 1 then
                        print(
                            "WARNING: Property", item.name .." from "..item.module.name..
                            " has a `@tparam` named \""..item.params[i].."\". The only"..
                            " valid name is \"..item.name\""
                        )
                    else
                        print(
                            "WARNING: Property", item.name .." from "..item.module.name..
                            " has a `@tparam` named \""..item.params[i].."\". This is"..
                            " not valid. It should be `@tparam[opt=...] <type> "..item.name..
                            ".<something> <description>"
                        )
                    end
                end
            end

            -- Some properties are table with fixed keys.
            local param,sublist = item:subparam(item.params[1])
            if sublist then
                for _, sub_param in pairs(param) do
                    local splitted = {}

                    for part in sub_param:gmatch("[^.]+") do
                        splitted[#splitted+1] = part
                    end

                    -- The first part should match the property name. If it doesn't,
                    -- then it's unclear what it is.
                    if splitted[1] ~= item.name then
                        print(
                            "WARNING: Property", item.name .." from "..item.module.name..
                            " has additional @tparams with unclear meaning. Please"..
                            " use `@tparam[opt=...] type property_name.subparam Desc...`"
                        )
                    else
                        local rebuilt_name = ""

                        for i=2, #splitted do
                            rebuilt_name = rebuilt_name .. ((#rebuilt_name > 0) and "." or "") .. splitted[i]
                        end

                        table_args[#table_args+1] = {
                            name        = rebuilt_name,
                            default     = item:default_of_param(sub_param),
                            type        = item:type_of_param(sub_param),
                            description = item.params.map[sub_param],
                        }
                    end
                end
            end

            if prop_has_obj and not prop_has_nil then
                print(
                    "WARNING: Property", item.name .." from "..item.module.name..
                    " has an object type, however it doesn't define the behavior"..
                    " or `nil` (or lack `@nonnullable`)"
                )
            end

            local type_map = {}

            for _, typeinfo in ipairs(metatypes) do
                local parsed = item.parsed_tags["propertytype"][typeinfo]
                type_map[parsed.typename.value] = type_map[parsed.typename.value] or {}
                type_map[parsed.typename.value][#type_map[parsed.typename.value]+1] = parsed
            end

            local warnings = {}

            -- Check if every possible type is described.
            for _, t in ipairs(prop_types) do
                if not type_map[t] then
                    -- Auto add description for common types.
                    if type_fallback_description[t] then
                        for _, entry in ipairs(type_fallback_description[t]) do
                            metatypes[#metatypes+1] = entry
                            named_tags.propertytype:add_to_item(item, entry)
                            type_map[t] = type_map[t] or {}
                            type_map[t][#type_map[t]+1] = entry
                        end
                    elseif #prop_types > 1 then
                        local warn = "WARNING: Property", item.name .." from "..item.module.name..
                            " has the undescribed object type \""..t.."\", please add"..
                            " one (or more) `@propertytype "..t.." <description>` tag."
                        warnings[t] = warnings[t] or {}
                        warnings[t][#warnings[t]+1] = warn
                    end
                end
            end

            -- Autofill the meaning of `nil` when there is a fallback `beautiful`
            -- variable.
            if prop_type_names["nil"]
              and (not type_map["nil"])
              and item.tags["propbeautiful"]
              and item.tags["propbeautiful"][1] then
                local modname = item.module.name:gmatch("[^.]+$")()
                local fallback = "beautiful."..(modname.."_"..item.name):gsub("[.]", "_")
                local entry = "nil Fallback to the current value of `"..fallback.."`."
                named_tags.propertytype:add_to_item(item, entry)
                metatypes[#metatypes+1] = entry

                warnings["nil"] = nil
            end

            for _, warn in ipairs(warnings) do
                print(warn)
            end

            local mt_by_type = {}

            -- Add the metatype section
            if #metatypes > 0 then
                local mt = item:add_metadata("Type description")

                for _, typeinfo in ipairs(metatypes) do
                    local parsed = item.parsed_tags["propertytype"][typeinfo]
                    local tmt = mt:add_metadata(
                        "<span class='summary_type'>"..parsed.typename.value.."</span>",
                        ldoc.markup(parsed.description)
                    )
                    mt_by_type[parsed.typename.value] = mt_by_type[parsed.typename.value] or {}
                    mt_by_type[parsed.typename.value][#mt_by_type[parsed.typename.value]+1] = tmt
                end
            end

            if #table_args > 0 then
                local mt = mt_by_type["table"] and mt_by_type["table"][1] or item:add_metadata("Table keys")
                for _, key in ipairs(table_args) do
                    mt:add_metadata(
                        key.name,
                        ldoc.markup(key.description),
                        key.type
                    )
                end
            end

            -- Add the function callback parameters section.
            if prop_type_names["function"] or fallback_callback then
                local fp = (mt_by_type["function"] and mt_by_type["function"][1] or item):add_metadata("Function prototype")

                local prototype = parse_properties_function_metadata(item, prop_type_names)

                if not prototype.has_input then
                    print(
                        "WARNING: Property ".. item.name .." from "..item.module.name..
                        " needs either `@functionparam` or  `@functionnoparam` if there"..
                        " is none."
                    )
                elseif #prototype.input == 0 then
                    fp:add_metadata("Parameters:", "The function has no parameters")
                else
                    local fparams = fp:add_metadata("Parameters")
                    for _, parsed in ipairs(prototype.input) do
                        if (not parsed.name) or (not parsed.type) then
                            print(
                                "WARNING: Property ".. item.name .." from "..item.module.name..
                                " has a `@functionparam` improperly formatted. The format is "..
                                 "`<type> <name> [description]`."
                            )
                        else
                            fparams:add_metadata(
                                parsed.name.value,
                                ldoc.markup(parsed.description),
                                parsed.type.value
                            )
                        end
                    end
                end

                args = item.tags["functionnoreturn"] and {} or item.tags["functionreturn"]

                if not prototype.has_output then
                    print(
                        "WARNING: Property ".. item.name .." from "..item.module.name..
                        " needs either `@functionreturn` or  `@functionnoreturn` if there"..
                        " is none."
                    )
                elseif #prototype.output == 0 then
                    fp:add_metadata("Return", "The function returns nothing.")
                elseif #prototype.output == 1 then
                    fp:add_metadata("Return", ldoc.markup(prototype.output[1].description), prototype.output[1].type.value)
                else
                    local md = fp:add_metadata("Return")
                    for _, parsed in ipairs(prototype.output) do
                        md:add_metadata(
                            "",
                            ldoc.markup(parsed.description),
                            parsed.type.value
                        )
                    end
                end
            end

            -- Handle the case where the table is a list.
            if #prop_types == 1 and prop_types[1] == "table"
              and #(item.tags["tablerowtype"] or {}) == 0
              and #table_args == 0 then
                print(
                    "WARNING: Property ".. item.name .." from "..item.module.name..
                    " needs either `@tablerowtype` or additional `@tparam`."
                )
            elseif #(item.tags["tablerowtype"] or {}) > 0 then
                local mt = item:add_metadata("Table content", ldoc.markup(item.tags["tablerowtype"][1]))

                for _, key in ipairs(item.tags["tablerowkey"] or {}) do
                    local parsed = item.parsed_tags["tablerowkey"][key]
                    local type, val, desc = parsed.type.value, parsed.name.value, parsed.description
                    mt:add_metadata("<code>"..val.."</code>", ldoc.markup(desc), type)
                end
            end
        end

        -- Allow the template to be shorter by using a for-loop.
        local has_start, has_allow_negative = false, false
        for _, mt in ipairs(metadata_tags) do
            local tag_desc = named_tags[mt]

            if item.tags[mt] and item.tags[mt][1] then
                has_start = has_start or mt == "rangestart"
                has_allow_negative = has_allow_negative or mt == "negativeallowed"
                local title = tag_desc.title or mt
                item:add_metadata(title, ldoc.markup(item.tags[mt][1]))
            end
        end

        if (prop_type_names["number"] or prop_type_names["integer"]) and not (has_start or has_allow_negative) then
            print(
                "WARNING: Property", item.name .." from "..item.module.name..
                " has numeric type, please add either `@rangestart` or `@negativeallowed false`."
            )
        end

        local tdesc = item.params.map[item.params[1]] and item.params.map[item.params[1]] or ""

        -- Auto add the description for "simple" boolean.
        if #prop_types == 1 and prop_types[1] == "boolean" and tdesc == "" then
            tdesc = "`true` or `false`."
        end

        -- Handle custom type description and string "enum".
        if #(item.tags["propertyvalue"] or {}) > 0 or tdesc ~= "" then

            local mt = item:add_metadata("Valid values", ldoc.markup(tdesc))

            local values, found_default = item.tags["propertyvalue"] or {}, false

            for _, enum_val in ipairs(values) do
                local parsed = item.parsed_tags["propertyvalue"][enum_val]
                local val, desc = parsed.value.value, parsed.description

                if val:sub(1,1) ~= '"' or val:sub(#val,#val) ~= '"' then
                    print(
                        "WARNING: Value `"..val.."` from property ".. item.name
                        .." from module "..item.module.name.. "should be a quoted string."
                    )
                end

                found_default = found_default or val == def

                mt:add_metadata("<code>"..val.."</code>", ldoc.markup(desc))
            end

            if def and #values > 0 and def:sub(1,9) ~= "beautiful" and not found_default then
                print(
                    "WARNING: Property ".. item.name .." from "..item.module.name..
                    " has some `@propertyvalue`, but the default value is not among them."
                )
            end
        end

        -- @return is not displayed for properties, something important
        -- might have been written there, so it's better to block it
        if item.tags["return"] or item.tags["treturn"] then
            print(
                "WARNING: Property ".. item.name .." from "..item.module.name..
                " has a `@return`, it is not rendered. Remove it."
            )
        end
    end
}

-- Documentation for objects deprecated properties
create_type {
    name           = "deprecatedproperty",
    title          = "Deprecated object properties",
    project_level  = false,
    subfield_title = "Type constraints",
}

-- Documentation for objects deprecated methods
create_type {
    name           = "deprecatedmethod",
    title          = "Deprecated object methods",
    project_level  = false,
    subfield_title = "Parameters",
}

-- Use a custom type for the methods to bypass the faulty ldoc built-in detection.
-- (yes, the space after Methods *is* on purpose to avoid clashing with ldoc
--  internal "methods" concept)
create_type {
    name           = "method",
    title          = "Object methods ",
    project_level  = false,
    subfield_title = "Parameters",
}

-- New type for signals.
local all_signals
all_signals = create_type {
    name           = "signal",
    title          = "Signals",
    project_level  = false,
    subfield_title = "Arguments",
    collection     = {},
    collect_callback = function(item, mod)
        collect_item_common(all_signals.collection, mod, item)
    end,
}

-- Deprecated signals.
create_type {
    name           = "deprecatedsignal",
    title          = "Deprecated signals",
    project_level  = false,
    subfield_title = "Arguments",
}

-- New type for signals connections
create_type {
    name           = "signalhandler",
    title          = "Request handlers",
    project_level  = false,
    subfield_title = "Arguments",
}

-- Allow objects to define a set of beautiful properties affecting them
all_theme_vars = create_type {
    name           = "beautiful",
    title          = "Theme variables",
    project_level  = false,
    subfield_title = "Type constraints",
    collection     = {},
    collect_callback = function(item, mod)
        if item.name:sub(1,4) ~= "beau" then
            -- This would be a bug.
            if item.name:match("[.]") then
                print("WANRING: A beautiful variable is called `", item.name, "`. This name is invalid.")
            end

            -- This happens because the `ldoc` messes with the name. All variables
            if item.module.name == "beautiful" then
                item.name = "beautiful." .. item.name
            else
                print(
                    "WARNING: All theme variable need a `beautiful.` prefix. It appears",
                    item.name,
                    " lacks it"
                )
            end
        end

        collect_item_common(all_theme_vars.collection, mod, item)

        -- Those are global variables, there can be only one per name.
        if all_theme_vars.collection[item.name] then
            local mod = pairs(all_theme_vars.collection[item.name])(all_theme_vars.collection[item.name])

            if mod ~= item.module.name then
                print("ERROR: "..item.name, "from", item.module.name, "is already defined in module ", mod)
            end
        end
    end,
    finish_callback = function(item)
        if not item.tags["beautiful_used_by"] then
            -- Every variable should be consumed by something. Better "park" some
            -- variable in the constructors than leave them disconnected from the
            -- global model.
            print("WARNING: ", item.name, "is not used by anything, add @usebeautiful or @propbeautiful")
        else
            local prop, mn = nil, item.module.name:match("[.]?([^.]+)$")
            -- If there is a property with the corresponding name and it doesn't
            -- mention the `beautiful` variable, that's nearly always a bug.
            for kind, items in item.module.kinds() do
                if kind == "Object properties" then
                    for k in items do
                        for k2,v2 in k do
                            if item.name:match("[.]"..mn.."_"..k2.name.."$") then
                                prop = k2
                                break
                            end
                            if prop then break end
                        end
                        break
                    end
                end
            end

            if prop then
                local mention = false

                if prop.tags.propbeautiful then mention = true end

                if (not mention) and prop:default_of_param(prop.params[1]) == item.name then
                    mention = true
                end

                for _, v in ipairs((not mention) and prop.tags.usebeautiful or {}) do
                    local parsed = prop.parsed_tags.usebeautiful[v]
                    if parsed.name.value == item.name then
                        mention = true
                        break
                    end
                end

                if not mention then
                    print(
                        "WARNING: `"..item.name.. "` from `"..item.module.name.."` "..
                        "seems to match a property called `"..prop.name.."`. However, "..
                        "there is no mention of this `beautiful` "..
                        "variable in its documentation. Please add `@propbeautiful` or "..
                        "`[opt="..item.module.name.."]"
                    )
                end
            end
        end
    end
}

-- Put deprecated methods in their own section
create_type {
    name           = "deprecated",
    title          = "Deprecated functions",
    project_level  = false,
    subfield_title = "Parameters",
}

-- For the legacy stateless layout related functions
create_type {
    name           = "legacylayout",
    title          = "Layout related functions",
    project_level  = false,
    subfield_title = "Parameters",
}

-- Have a category for the client layouts
create_type {
    name           = "clientlayout",
    title          = "Client layouts",
    project_level  = false,
    subfield_title = "Parameters",
}

-- Source functions for the taglist/tasklist/layoutlist
create_type {
    name          = "sourcefunction",
    title         = "List source functions",
    project_level = false,
}

-- Document some callback prototypes
create_type {
    name           = "callback",
    title          = "Callback functions prototype",
    project_level  = false,
    subfield_title = "Parameters",
}

-- gears.matcher / awful.rules sources
create_type {
    name           = "rulesources",
    title          = "Rule sources",
    project_level  = false,
    subfield_title = "Parameters",
}

-- gears.matcher / awful.rules rule components
create_type {
    name           = "rulecomponent",
    title          = "Rule components",
    project_level  = false,
    subfield_title = "Type",
}

-- Filter functions for the taglist/tasklist/layoutlist
create_type {
    name          = "filterfunction",
    title         = "List filters",
    project_level = false,
}

-- Extra client properties available only in awful.rules/spawn constructs
create_type {
    name           = "clientruleproperty",
    title          = "Extra properties available in the rules",
    project_level  = false,
    subfield_title = "Type",
}

-- Extra *matching* properties for rules.
create_type {
    name           = "matchingproperty",
    title          = "Extra matching properties used in rules",
    project_level  = false,
    subfield_title = "Type",
}

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
local function default_format(self, callback, name)
    return function(raw, item, md)
        local p = name and item.parsed_tags and item.parsed_tags[name] and item.parsed_tags[name][raw]

        if not p then
            p = parse_custom_tags(raw, self.params or {})
        end

        return (callback or default_format_callback)(self, p, item, md)
    end
end

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

    databases[name] = databases[name] or {}
    databases[name].collect = args.collect_callback

    local f = args.format

    args.format = default_format(args, f, name)

    function args:add_to_item(item, tag_value)
        add_custom_tag_common(item, name, args, tag_value)

        local parsed = parse_custom_tags(tag_value, args.params or {})

        if parsed then
            item.parsed_tags = item.parsed_tags or {}
            item.parsed_tags[name] = item.parsed_tags[name] or {}
            item.parsed_tags[name][tag_value] = parsed
        end

        item.has_show_more = item.has_show_more or (not args.hidden)

        return parsed
    end

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
    },
    collect_callback = function(item, mod, tag, value)
        local parsed = parse_custom_tags(value, named_tags.emits.params)

        all_signals.collection[parsed.name.value] = all_signals.collection[parsed.name.value] or {}

        local var = all_signals.collection[parsed.name.value]

        var[mod.name] = var[mod.name] or {}
        var[mod.name][#var[mod.name] + 1] = item
    end
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
    },
    collect_callback = function(item, mod, tag, value)
        local params = parse_custom_tags(value, named_tags.usebeautiful.params)

        if all_theme_vars.collection[params.name.value] then
            local sig_mods = all_theme_vars.collection[params.name.value]
            local _, themed_items = pairs(sig_mods)(sig_mods)
            local themed_item = themed_items[1]

            named_tags.beautiful_used_by:add_to_item(themed_item, item.name.." "..item.summary)

            -- Auto fill the description if its empty.
            if (not params.description) or params.description == "" then
                params.description = themed_item.summary
            end
        else
            print("WARNING: Could not find theme variable", params.name.value)
        end
    end
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
    end,
    finish_callback = function(item, raw, params, modules)
        local modname = item.module.name:gmatch("[^.]+$")()
        local name = "beautiful."..(modname.."_"..item.name):gsub("[.]", "_")

        if all_theme_vars.collection[name] then
            local sig_mods = all_theme_vars.collection[name]
            local _, themed_items = pairs(sig_mods)(sig_mods)
            local themed_item = themed_items[1]

            named_tags.beautiful_used_by:add_to_item(themed_item, item.name.." "..item.summary)
        end
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
}

-- Mark the item as readonly.
-- This tag should be used to mark readonly properties.
-- @readonly
add_custom_tag {
    name = "readonly",
    hidden = true,
    auto_subtags = false
}

-- Forces every method and function to have either `@treturn` or `@noreturn`.
-- This avoids the many case where the return value was forgotten.
add_custom_tag {
    name = "noreturn",
    hidden = true,
    auto_subtags = false
}

-- When a property cannot be `nil` or a function/method cannot return `nil`.
add_custom_tag {
    name = "nonnullable",
    hidden = true,
    auto_subtags = false
}

-- When properties are integers, the value usually has a meaning, like the PID,
-- apoint or a pixel.
add_custom_tag {
    name = "propertyunit",
    title = "Unit",
    hidden = true,
    auto_subtags = false
}

-- Some string properties are de-facto enums. Only a small set of value is valid.
-- This tag provides a consistent rendering for such properties.
add_custom_tag {
    name = "propertyvalue",
    hidden = true,
    auto_subtags = false,
    params = {
        { name = "value" },
    },
}

-- Some defaults are not values. They are either algorithms, chains od fallbacks
-- or somehow inherited. `ldoc` `opt` can only parse single "words", thus a new
-- tag is needed for textual description of the default value.
add_custom_tag {
    name = "propertydefault",
    hidden = true,
    auto_subtags = false,
}

-- Some properties have multiple possible types. Some also might support multiple
-- "logical type" for the same datatype. For example `string` can be a path or
-- some CSS/SVG or an integer have different meaning for positive vs.
-- negative/zeroed.
--
-- This tag helps document those nuances.
add_custom_tag {
    name = "propertytype",
    hidden = true,
    auto_subtags = false,
    params = {
        { name = "typename" },
    },
}

-- Some values, mostly bytes, have a minimum and maximum value.
add_custom_tag {
    name = "rangestart",
    title = "Minimum value",
    hidden = true,
    auto_subtags = false
}
add_custom_tag {
    name = "rangestop",
    title = "Maximum value",
    hidden = true,
    auto_subtags = false
}
add_custom_tag {
    name = "negativeallowed",
    title = "Negative allowed",
    hidden = true,
    auto_subtags = false,
    params = {
        { name = "allowed" },
    },
}

-- A lot of properties have the `table` value. Lua tables can be anything from
-- "struct" to list to objects. For lists, then the type of the row needs to
-- be specified.
add_custom_tag {
    name = "tablerowtype",
    title = "Table content",
    hidden = true,
    auto_subtags = false,
}
add_custom_tag {
    name = "tablerowkey",
    title = "Row keys",
    hidden = true,
    auto_subtags = false,
    params = {
        { name = "type" },
        { name = "name" },
    },
}

-- When a `@property` takes a `function` value, then the function arguments
-- much be provided. If there is none, then `@functionnoparam` must be explicitly
-- specified. Same for the return value(s).
add_custom_tag {
    name = "functionnoparam",
    hidden = true,
    auto_subtags = false,
}
add_custom_tag {
    name = "functionparam",
    hidden = true,
    auto_subtags = false,
    params = {
        { name = "type" },
        { name = "name" },
    },
}
add_custom_tag {
    name = "functionnoreturn",
    hidden = true,
    auto_subtags = false,
}
add_custom_tag {
    name = "functionreturn",
    hidden = true,
    auto_subtags = false,
    params = {
        { name = "type" },
    },
}

add_custom_tag {
    name   = "signalhandler",
    hidden = false,
    title  = "Request handler",
    auto_subtags = false,
    params = {
        { name = "source" },
    },
    format = function(self, params, item, md)
        return ldoc.markup("`"..params.source.value.."`") .. " " .. params.description
    end
}

add_custom_tag {
    name = "beautiful_used_by",
    hidden = false,
    title = "Used by",
    auto_subtags = false,
    params = {
        { name = "source" },
    },
    format = function(self, params, item, md)
        return ldoc.markup("`"..params.source.value.."`") .. " " .. params.description
    end
}

-- Auto generate some code for replacing request handlers.
add_custom_tag {
    name   = "sourcesignal",
    title  = "Source signal",
    hidden = false,
    params = {
        { name = "class"   },
        { name = "signal"  },
    },
    format  = function(self, params, item, md)
        return ldoc.markup("`"..params.signal.value.."` from the `"..params.class.value.."` module.")..
            " This code allows to disconnect this handler and implement your own:"
    end,
    finish_callback = function(item, raw, params, modules)
        if all_signals.collection[params.signal.value] then
            local sig_mods = all_signals.collection[params.signal.value]
            local _, signal_items = pairs(sig_mods)(sig_mods)
            local signal_item = signal_items[1]

            -- Auto generate some code.
            local code = "-- Disconnect the original handler.\n"
            code = code .. signal_item.module.name .. ".disconnect_signal('".. params.signal.value .."', "..item.name..")\n\n"
            code = code .. "-- Connect a custom handler.\n"
            code = code .. signal_item.module.name .. ".connect_signal('".. params.signal.value .."', function("


            if signal_item.type == "signal" then
                for idx, parm in ipairs(signal_item.params) do
                    code = code .. parm .. (idx == #signal_item.params and "" or ", ")
                end

                code = code .. ")\n    -- code here\nend)"

                named_tags.signalhandler:add_to_item(signal_item, item.name.." "..item.summary)
            end

            item.tag_extra_data = item.tag_extra_data or {}
            item.tag_extra_data["sourcesignal"] = item.tag_extra_data["sourcesignal"] or {}
            item.tag_extra_data["sourcesignal"][raw] = item.tag_extra_data["sourcesignal"][raw] or {}
            item.tag_extra_data["sourcesignal"][raw].usage = ldoc.prettify(code)

            item.has_show_more = true
        end
    end
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
        '../lib/wibox/drawable.lua',
    }
}

-- Wrap the module name for the CSS highlight.
local function wrap_modname(str, item)
    if (not item.module) or str:sub(1, #item.module.name+1) ~= item.module.name.."." then return str end

    return "<span class='function_modname'>"..item.module.name..".</span>"
        .. str:sub(#item.module.name+2, 9999)
end

local named_args = {
    [ "(args)"                                     ] = true,
    [ "([args])"                                   ] = true,
    [ "([args=nil])"                               ] = true,
    [ "([args={}])"                                ] = true,
    [ '(<span class="optional_param">args</span>)' ] = true,
}

-- Some values come from external sources, we can't enforce the naming conventions
-- on them. So far, they are all keyboard related.
local param_name_whitelist = {
    Mod2 = true, Lock = true, Control = true, Mod1 = true, ISO_Level3_Shift = true,
    Mod4 = true, Insert = true, Delete = true, Next = true, Prior = true, Left = true,
    Up = true, Right = true, Down = true, KP_End = true, KP_Down = true,
    KP_Next = true, KP_Left = true, KP_Begin = true, KP_Right = true,
    KP_Home = true, KP_Up = true, KP_Prior = true, KP_Insert = true, KP_Delete = true,
    KP_Divide = true, KP_Multiply = true, KP_Subtract = true, KP_Add = true,
    KP_Enter = true, Escape = true, Tab = true, space = true, Return = true,
    XF86MonBrightnessUp = true, XF86MonBrightnessDown = true,
    XF86AudioRaiseVolume = true, XF86AudioLowerVolume = true, XF86AudioMute = true,
    XF86AudioPlay = true, XF86AudioPrev = true, XF86AudioNext = true, XF86AudioStop = true,
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
    sourcesignal  = { "div", "div", "span", "span" }
}

--- If the title is short and descritive, there is no point to bloat it.
--
-- That test is mostly to catch empty or lazy description (such as the module
-- name and nothing else).
local short_title_whitelist = {
    [ "rc.lua"                  ] = true,
    [ "naughty.action"          ] = true,
    [ "naughty.layout.box"      ] = true,
    [ "naughty.widget.title"    ] = true,
    [ "theme.lua"               ] = true,
    [ "wibox.container.margin"  ] = true,
    [ "wibox.container.arcchart"] = true,
    [ "wibox.widget.checkbox"   ] = true,
    [ "wibox.widget.imagebox"   ] = true,
    [ "wibox.widget.separator"  ] = true,
    [ "wibox.widget.progressbar"] = true,
    [ "naughty"                 ] = true,
    [ "xproperties"             ] = true,
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

    local ret, has_show_more = {}, item.has_show_more or false

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

    item.delims, item.auto_usage, item.auto_params, item.metadata = {}, {}, {}, {}

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
        local extra = (item.tag_extra_data and item.tag_extra_data[tag] and item.tag_extra_data[tag][value]) or {}

        -- Makes auto-generated subtags easier to implement.
        if item.auto_params[tag.."tparam_override"] then
            return item.auto_params[tag.."tparam_override"], named_tags[tag.."tparam"], extra
        end

        local parsed = nil

        if item.parsed_tags and item.parsed_tags[tag] and item.parsed_tags[tag][value] then
            parsed = item.parsed_tags[tag][value]
        end

        if not item.auto_params[tag.."tparam"] then return nil, nil, extra end

        if parsed.name and item.auto_params[tag.."tparam"][parsed.name.value] then
            return item.auto_params[tag.."tparam"][parsed.name.value], named_tags[tag.."tparam"], extra
        end

        return nil, nil, extra
    end

    -- Recursive way to annotate property-like objects.
    local amt
    amt = function(self, title, description, datatype)
        self.metadata[#self.metadata+1] = {
            title        = title,
            datatype     = datatype,
            description  = description,
            metadata     = {},
            add_metadata = amt
        }
        return self.metadata[#self.metadata]
    end
    item.add_metadata = amt

    item.is_init = true
end

-- Wrap the arguments for the CSS highlight.
local function wrap_args(item)
    if not item.args then return "" end

    -- Display named args with `{}` and ordered args with `()`
    if named_args[item.args] then
        item.is_named_call = true
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
local function sanitize_type(item)
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

-- Detect error prone markdown formatting.
local function detect_markdown_footguns(string)
    for line in string:gmatch("[^\n]*") do
        if line:sub(1,4) ~= "    " then
            local is_tick = false

            -- Detect unquoted code with 2 underscores. They will render as italic,
            -- which is definitively not wanted. Add "`" to fix.
            for part in line:gmatch("[^`]*`?") do
                if not is_tick then
                    local count = 0

                    for under in part:gmatch("[^\\]_") do
                        count = count + 1
                    end

                    if count > 1 then
                        return false
                    end
                end

                is_tick = not is_tick
            end
        end
    end

    return true
end

-- Add parentheses when there is alternative return types.
local function generate_return_tuple(item, group)
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
local function sanitize_return_type(item)

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
    elseif not item.tags["noreturn"] then
        print(
            "WARNING:", item.name, "from", item.module.name,
            "doesn't have a return value or @noreturn"
        )
    end

    item.compact_signature = true
end

-- Traverse the entire doc and reverse-map the links between our custom
-- tags and the content it is pointing toward. From that, we add some
-- "consumers"/"providers" tables to each (relevant) item.
local function global_init(_ldoc)
    ldoc = _ldoc

    if is_init then return end
    is_init = true

    local module_by_name = {}

    -- First pass, gather the providers.
    for _, mod in ipairs(ldoc.modules) do
        module_by_name[mod.name] = mod

        if mod.type ~= "topic" and ((not mod.summary) or mod.summary == "" or mod.summary:lower():sub(1,8) == "awesome") then
            print("WARNING: Each module needs a good summary as its first line", mod.name, "doesn't")
        end

        local sum_count = 0

        for line in (mod.summary or ""):gmatch("[^\n]*") do
            sum_count = sum_count + 1
        end

        if sum_count > 1 and #mod.summary > 120 then
            print("WARNING:", mod.name, "summary has to be only 1 line.")
        end

        if mod.summary and #mod.summary < 30 and not short_title_whitelist[mod.name] then
            print("WARNING:", mod.name, "summary is too short.")
        end

        for tag, values in pairs(mod.tags) do
            for _, value in ipairs(values.gmatch and {values} or values) do
                -- Collect instances of some types.
                if databases[tag] and databases[tag].collect then
                    delayed_collect[#delayed_collect+1] = {mod, mod, tag, value}
                end
            end
        end

        for kind, items in mod.kinds() do
            for item in items() do
                -- Decorate the item with our customizations.
                init_custom_types(item)

                -- print(item.description)

                if item.summary and not detect_markdown_footguns(item.summary) then
                    print(
                        "WARNING: "..item.name.." from "..item.module.name.." seems to have"..
                        " ambiguous markdown in its summary, probably underscores. Please either"..
                        " use '`' or HTML."
                    )
                end

                -- Collect instances of some types.
                if databases[item.type] and databases[item.type].collect then
                    databases[item.type].collect(item, mod)
                end

                for tag, values in pairs(item.tags) do
                    for _, value in ipairs(values.gmatch and {values} or values) do
                        -- Collect instances of some types.
                        if databases[tag] and databases[tag].collect then
                            delayed_collect[#delayed_collect+1] = {item, mod, tag, value}
                        end
                    end
                end

                -- Lint the parameters too.
                for parm in ldoc.modules.iter(item.params) do
                    local first = parm:match("^[ ]*(.)")

                    -- If the first letter of a parameter is capital, the type is probably missing.
                    -- That's what happen if you rename @param to @tparam without actually adding the type.
                    if first and first ~= first:lower() and parm:upper() ~= parm and not param_name_whitelist[parm] then
                        print(
                            "ERROR: Parameter "..parm.." from ".. item.name .. " ".. item.module.name..
                            " starts with a capital letter. This is forbidden."
                        )
                    end

                    for p in ldoc.modules.iter(item:subparam(parm)) do
                        local desc = item.params.map[p]

                        if not detect_markdown_footguns(desc) then
                            print(
                                "WARNING: Always use '`' to encapsulate code in the parameters." ..
                                p .. " from " .. item.name .. " " .. item.module.name .. " contains " ..
                                "some ambiguous markup. If this is a false positive, use HTML."
                            )
                        end

                        -- It *sometime* works, but is at risk if being  interpreted as code by markdown.
                        if desc:match("^ [ ]+") then
                            print(
                                "WARNING: ", item.name, "from", item.module.name, "has a parameter description "..
                                "with leading spaces. This will not render correctly", "'"..desc.."'"
                            )
                        end

                        first = desc:match("^[ ]*([a-z])")

                        if first and first ~= first:upper() then
                            print("WARNING: Sentences starts with a capital letter. Please fix", item.module.name, item.name)
                        end

                        --TODO convert @propbeautiful to @usebeautiful and lint them
                        --TODO check the module summary.
                    end
                end
            end
        end
    end

    -- Second pass, collect the custom tags. This is delayed because
    -- the tags depend on the items. Not all items have been processed if
    -- this was called during the first pass. Also, collect callbacks can
    -- themselves create more delayed_collect entries recursively.
    for _, to_collect in ipairs(delayed_collect) do
        databases[to_collect[3]].collect(to_collect[1], to_collect[2], to_collect[3], to_collect[4])
    end

    -- Third pass, parse every custom tags and fill the consumers tables.
    for _, mod in ipairs(ldoc.modules) do
        for kind, items in mod.kinds() do
            for item in items() do
                for tag_name, values in pairs(item.tags) do
                    for _, tag in ipairs(values.gmatch and {values} or values) do
                        local parsed = named_tags[tag_name]
                            and named_tags[tag_name].params
                            and parse_custom_tags(tag, named_tags[tag_name].params)

                        -- Allow the tags to, ie, auto-generate some extra content.
                        if named_tags[tag_name] and named_tags[tag_name].finish_callback then
                            named_tags[tag_name].finish_callback(item, tag, parsed, module_by_name)
                        end

                        -- Save the parsed content to be used in get_auto_params.
                        if parsed then
                            item.parsed_tags = item.parsed_tags or {}
                            item.parsed_tags[tag_name] = item.parsed_tags[tag_name] or {}
                            item.parsed_tags[tag_name][tag] = parsed
                        end
                    end
                end

                -- Mostly for linting.
                if databases[item.type] and databases[item.type].finish then
                    databases[item.type].finish(item, mod, module_by_name)
                end
            end
        end
    end
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
    signalhandler   = true,
    table           = true,
}

-- Add the arguments.
local add_args = {
    constructorfct  = true,
    constructorfct2 = true,
    staticfct       = true,
    signalhandler   = true,
    callback        = true,
    deprecated      = true,
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
    item.global_init = global_init

    if is_deprecated[item.type] then
        item.tags.is_deprecated = {true}
    end

    init_custom_types(item)

    -- Do not use the default handler. It encodes the parameters using
    -- the optchain convention. Not everybody is confortable with this.
    -- It is also unreadable.
    item.args = ""

    if item.params then
        item.args = "("
        for key, line in ipairs(item.params) do
            local name, comment = item:split_param(line)
            local def = item:default_of_param(line)
            if def then
                item.args = item.args .. '<span class="optional_param">' .. name .. "</span>" .. (key < #item.params and ", " or "")
            else
                item.args = item.args .. name .. (key < #item.params and ", " or "")
            end
        end
        item.args = item.args .. ")"
    end

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
