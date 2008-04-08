#!/usr/bin/env python
#
# extractuicbdoc.py - extract uicb documentation from awesome source code
# Copyright (C) 2008 Marco Candrian <mac@calmar.ws>
#
# This indeed crappy. Any better version, even with awk, would be welcome.


print """Note: when there is no whitespace, quotes are optional.

<boolean>       -> "true" or "false"
<color>         -> #ff9933 (hexadecimal color notation: #red green blue)
<float>         -> 0.3, 0,8 (often values between 0 and 1 are useful)
<font>          -> Pango font: sans 10, sans italic 10, fixed 12, ...
<identifier>    -> foobar (choose a name/string)
<image>         -> "/home/awesome/pics/icon.png" (path to image)
<integer>       -> 1, 10, -3 (positive numbers are required mostly)
<key>           -> a, 1, F10 (see /usr/include/X11/keysymdef.h w/o XK_ or 'xev')
                   or a keycode beginning with #
<mod>           -> "Mod1", "Mod4", "Control" (modifiers)
<regex>         -> regular expression
<string>        -> "foo bar"
<uicb-arg>      -> prog, 3... (argument to a uicb function, where required)
<uicb-cmd>      -> spawn, exec, client_tag... (see UICB FUNCTIONS above)
<position>      -> list of position: off, top, right, left, bottom, auto
<style section> -> a section with font, fg, bg, border, shadow and shadow_offset options.
                   { font = <font> fg = <color> bg = <color> border = <color>
                     shadow = <color> shadow_offset = <integer> }
<titlebar sec>  -> a section with position and icon position.
                   { position = <position> icon = <position> text_align = <{center,right,left}>
                     height = <integer> width = <integer> styles { } }
<{.., ...}>     -> list of available options

[MULTI] means, you can use an item multiple times.
"""
#<layout>        -> dwindle, floating, max, spiral, tile, tileleft, tilebottom, tiletop

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
            elif sections[sec][opt][9:] == "titlebar_opts":
                print indent + "<titlebar section>"
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
            elif opt in ["layout"]:
                print indent + opt + " = <layout>"
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
                section_doc[option_title] = "<alignment>"
            elif line.startswith("    CFG_POSITION"):
                section_doc[option_title] = "<position>"
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
