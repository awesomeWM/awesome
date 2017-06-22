#!/bin/sh

die()
{
	exec >&2
	echo
	echo
	echo "       WARNING"
	echo "       ======="
	echo
	echo " The lgi check failed."
	echo " The Lua GObject introspection package is just a runtime dependency, so it is not"
	echo " necessary for building awesome. However, awesome needs it to run."
	echo " Add AWESOME_IGNORE_LGI=1 to your environment to continue."
	echo
	echo
	if [ "x$AWESOME_IGNORE_LGI" = "x1" ]
	then
		exit 0
	fi
	exit 1
}

# Check if we have lgi
lua -e 'require("lgi")' || die

# Check the version number.
# Keep this in sync with lib/gears/init.lua and .travis.yml (LGIVER)!
lua -e '_, _, major_minor, patch = string.find(require("lgi.version"), "^(%d%.%d)%.(%d)");
	if tonumber(major_minor) < 0.8 or (tonumber(major_minor) == 0.8 and tonumber(patch) < 0) then
		error(string.format("lgi is too old, need at least version %s, got %s.",
		                    "0.8.0", require("lgi.version"))) end' || die

# Check for the needed gi files
lua -e 'l = require("lgi") assert(l.cairo, l.Pango, l.PangoCairo, l.GLib, l.Gio)' || die

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
