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

# Check the version number
# Keep this in sync with lib/gears/surface.lua.in!
lua -e 'if tonumber(string.match(require("lgi.version"), "(%d%.%d)")) < 0.7 then error("lgi too old, need at least version 0.7.0") end' || die

# Check for the needed gi files
lua -e 'l = require("lgi") assert(l.cairo, l.Pango, l.PangoCairo)' || die
