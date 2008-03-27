#!/usr/bin/python
#
# extractuicbdoc.py - extract uicb documentation from awesome sour code
# Copyright (C) 2008 Julien Danjou <julien@danjou.info>
#
# This indeed crappy. Any better version, even with awk, would be welcome.
#

import sys
import os.path

def extract_doc(file):
    function_doc = {}
    doc = []
    doc_begin = False
    catch_name = 0
    
    for line in file.readlines():
        if catch_name == 1:
            catch_name = 2
            continue
        if catch_name > 1:
            # We only want functions with uicb
            if line.startswith("uicb_"):
                if line.endswith("arg __attribute__ ((unused)))\n"):
                    doc.append("No argument needed.")
                uicb_name = line.split('_', 1)[1]
                uicb_name = uicb_name.split('(', 1)[0]
                function_doc[uicb_name] = doc
                catch_name = False
            else:
                catch_name = 0
            doc = []
        if line.startswith("/**"):
            doc_begin = True
            doc.append(line[4:].replace("\n", ""))
        if doc_begin and line.startswith(" * ") and not line.startswith(" * \\"):
            doc.append(line[3:].replace("\n", ""))
        if line.startswith(" */"):
            doc_begin = False
            catch_name = 1

    return function_doc

def doc_print(function_doc, filename):
    if not len(function_doc):
        return
    filename = os.path.basename(filename)
    # Special case
    if filename == "uicb.c":
        filename = "general.c"
    # Print header
    tmptitle = filename.replace(".c", "")
    title = tmptitle[0].upper() + tmptitle[1:]
    print title
    i = 0
    underline = ""
    while i < len(title):
        underline += "~"
        i += 1
    print underline
    # Print documentation
    for uicb, doc in function_doc.items():
        # Special case
        print "*%s*::" % uicb
        print "    %s" % " ".join(doc)
    print

for f in sys.argv[1:]:
    doc_print(extract_doc(file(f)), f)
