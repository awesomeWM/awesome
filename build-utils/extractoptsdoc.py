#!/usr/bin/env python
#
# extractoptsdoc.py - extract options documentation from awesome sour code
# Copyright (C) 2008 Julien Danjou <julien@danjou.info>
#
# This indeed crappy. Any better version, even with awk, would be welcome.
#

import sys

def section_print(section, options):
    print section
    i = 0
    underline = ""
    while i < len(section):
        underline += "~"
        i += 1
    print underline
    if options.has_key("comments"):
        print "%s\n" % options.pop("comments")
    for option, format in options.items():
        print "%s::" % option
        print "    %s" % format
    print

def sections_print(sections):
    section_print("Base sections", sections.pop("awesome"))
    keylist = sections.keys()
    keylist.sort()
    for key in keylist:
        section_print(key, sections[key])

def sections_get(file):
    sections = {}
    section_doc = {}
    section_begin = False
    for line in file.readlines():
        if line.startswith("cfg_opt_t"):
            section_name = (line.split(" ", 1)[1]).split("_opts")[0]
            section_doc['comments'] = lastline[4:-3]
            section_begin = True
        elif section_begin and line.startswith("};"):
            section_begin = False
            sections[section_name] = section_doc
            section_doc = {}
        elif section_begin and line.startswith("    CFG_"):
            if line.startswith("    CFG_AWESOME_END"):
                continue
            option_title = line.split("\"")[1].split("\"")[0]
            if lastline.startswith("    /**"):
                section_doc[option_title] = lastline[8:-3]
            else:
                section_doc[option_title] = "Undocumented option. "
            if line.startswith("    CFG_INT"):
                section_doc[option_title] += "This option must be an integer value."
            elif line.startswith("    CFG_BOOL"):
                section_doc[option_title] += "This option must be a boolean value."
            elif line.startswith("    CFG_FLOAT"):
                section_doc[option_title] += "This option must be a float value."
            elif line.startswith("    CFG_ALIGNMENT"):
                section_doc[option_title] += "This option must be an alignment value."
            elif line.startswith("    CFG_POSITION"):
                section_doc[option_title] += "This option must be a position value."
            elif line.startswith("    CFG_STR_LIST"):
                section_doc[option_title] += "This option must be string list."
            elif line.startswith("    CFG_STR"):
                section_doc[option_title] += "This option must be string value."
            elif line.startswith("    CFG_SEC"):
                secname = (line.split(", ")[1]).split("_opts", 1)[0]
                section_doc[option_title] += "This option must be a section `%s'" % secname
                if line.find("CFGF_MULTI") != -1:
                    section_doc[option_title] += ", can be specified multiple times"
                if line.find("CFGF_NO_TITLE_DUPES") != -1:
                    section_doc[option_title] += ", must have a unique title"
                elif line.find("CFGF_TITLE") != -1:
                    section_doc[option_title] += ", must have a title"
                section_doc[option_title] += "."

        lastline = line

    return sections

sections_print(sections_get(file(sys.argv[1])))
