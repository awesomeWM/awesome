#!/usr/bin/env python
#
# extractuicbdoc.py - extract uicb documentation from awesome source code
# Copyright (C) 2008 Julien Danjou <julien@danjou.info>
# Copyright (C) 2008 Marco Candrian <mac@calmar.ws>
#
# This indeed crappy. Any better version, even with awk, would be welcome.


print """Note: when there is no whitespace, quotes are optional.

<boolean>       -> true or false
<color>         -> Color in X format or hexadecimal (e.g. #aabbcc)
<float>         -> Floating numbers (e.g 0.2)
<font>          -> Pango font: [FAMILY-LIST] [STYLE-OPTIONS] [SIZE] (e.g Sans Italic 12)
<identifier>    -> A name used to identify (e.g foobar)
<image>         -> A path to an image (e.g. /home/user/image.jpg)
<integer>       -> A signed integer
<key>           -> A KeySym (e.g. F10) or a KeyCodea (e.g #120)
<mod>           -> A key modifier list (e.g. Mod1)
<regex>         -> Regular expression
<string>        -> A string
<string-list>   -> A string list (e.g. {a, b, c, ...})
<uicb-arg>      -> Argument to an uicb function
<uicb-cmd>      -> Uicb function, see UICB FUNCTIONS
<style section> -> A style section: {fg= bg= border= font= shadow= shadow_offset= }
<{.., ...}>     -> List of available options
[MULTI]         -> This item can be defined multiple times
"""

import sys

indent = ""
section_desc = {} # [original_section_name]  -> description
sections = {} # [original_section_name]  -> items inside

def sections_print(sec):
    global sections, section_desc, indent
    seclist = sections[sec].keys()
    seclist.sort()
    for opt in seclist:
        if sections[sec][opt][:9] == "<SECTION>":
            print indent + opt + section_desc[sections[sec][opt][9:]]
            print indent + "{"
            indent += "    "
            if sections[sec][opt][9:] == "style_opts":
                print indent + "<style section>"
            else:
                sections_print(sections[sec][opt][9:])
            indent = indent[0:-4]
            print indent + "}"
        else:
            if opt in ["fg", "bg", "fg_center", "fg_end", "fg_off", "bordercolor",
                       "tab_border","normal_fg","normal_bg","normal_border","focus_fg",
                       "focus_bg","focus_border","urgent_fg","urgent_bg"]:
                print indent + opt + " = <color>"
            elif opt in ["font"]:
                print indent + opt + " = <font>"
            elif opt in ["image"]:
                print indent + opt + " = <image>"
            elif opt in ["name", "xproperty_value", "tags"]:
                print indent + opt + " = <regex>"
            elif opt in ["key"]:
                print indent + opt + " = <key>"
            elif opt in ["keylist"]:
                print indent + opt + " = <key, ...>"
            elif opt in ["modkey"]:
                print indent + opt + " = <mod>"
            elif opt in ["align"]:
                print indent + opt + " = <{auto, left, right}>"
            elif opt in ["text_align"]:
                print indent + opt + " = <{left, center, right}>"
            elif opt in ["arg"]:
                print indent + opt + " = <uicb-arg>"
            elif opt in ["command"]:
                print indent + opt + " = <uicb-cmd>"
            elif opt in ["float", "master"]:
                print indent + opt + " = <{auto,true,false}>"
            elif opt in ["show"]:
                print indent + opt + " = <{all,tags,focus}>"
            elif opt in ["floating_placement"]:
                print indent + opt + " = <{smart,under_mouse}>"
            else:
                print indent + opt + " = " + sections[sec][opt]

def sections_get(file):
    global sections, section_desc
    section_doc = {} # holds all items from a section
    section_begin = False
    for line in file.readlines():
        if line.startswith("cfg_opt_t"):
            section_name = (line.split(" ", 1)[1]).split("[]")[0]
            section_begin = True
        elif section_begin and line.startswith("};"):
            section_begin = False
            sections[section_name] = section_doc
            section_doc = {}
        elif section_begin and line.startswith("    CFG_"):
            if line.startswith("    CFG_AWESOME_END"):
                continue
            option_title = line.split("\"")[1].split("\"")[0]

            if line.startswith("    CFG_INT"):
                section_doc[option_title] = "<integer>"
            elif line.startswith("    CFG_BOOL"):
                section_doc[option_title] = "<boolean>"
            elif line.startswith("    CFG_FLOAT"):
                section_doc[option_title] = "<float>"
            elif line.startswith("    CFG_ALIGNMENT"):
                section_doc[option_title] = "<{left, center, right, flex, auto}>"
            elif line.startswith("    CFG_POSITION"):
                section_doc[option_title] = "<{top, bottom, left, right, auto, off}>"
            elif line.startswith("    CFG_STR_LIST"):
                section_doc[option_title] = "<string-list>"
            elif line.startswith("    CFG_STR"):
                section_doc[option_title] = "<string>"
            elif line.startswith("    CFG_SEC"):
                secname = (line.split(", ")[1]).split(",", 1)[0]
                str = ""
                if line.find("CFGF_NO_TITLE_DUPES") != -1:
                    str += " <identifier>"
                elif line.find("CFGF_TITLE") != -1:
                    str += " <title>"
                if line.find("CFGF_MULTI") != -1:
                    str += " [MULTI]"
                section_desc[secname] = str
                section_doc[option_title] = "<SECTION>" + secname

        lastline = line

sections_get(file(sys.argv[1]))

sections_print("awesome_opts")

# vim: filetype=python:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
