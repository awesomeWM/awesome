# Fedora Installation Readme

## Build Requirements

### Base

#### Core
* git (you should have this already)
* cmake >= 3.0.0
* lua-devel >= 5.1
* lua-lgi >= 0.7.1
* ImageMagick (for convert)
* desktop-file-utils (for verifying the .desktop file)

#### Devel Needs
* libxcb-devel >= 1.6
* glib2-devel
* gdk-pixbuf2-devel
* cairo-devel
* cairo-goject-devel
* xcb-util-devel >= 0.3.8
* xcb-util-cursor-devel
* xcb-util-keysyms-devel >= 0.3.4
* xcb-util-wm-devel >= 0.3.8 (package for icccm)
* xcb-util-xrm-devel >= 1.0
* libxkbcommon-devel
* libxkbcommon-x11-devel
* libxdg-basedir-devel >= 1.0.0
* startup-notification-devel >= 0.10
* pango-devel >= 1.19.3
* dbus-devel

### Documentation

* lua-ldoc

### Manpages

* asciidoc
* xmlto
* gzip

### Extra

* xterm (default terminal)
* nano (default editor)

## DNF install

If you wish to make your rpm on your system, you can copy/paste this line and read the Make section below. If you don't wish to install all these packages on your system, you can do a `mock` build instead which won't install to your system, but to a contained system. Read the Mock section below for instructions.

`dnf install git cmake lua-devel lua-lgi ImageMagick asciidoc xmlto gzip lua-ldoc libxcb-devel glib2-devel gdk-pixbuf2-devel cairo-devel xcb-util-cursor-devel xcb-util-devel xcb-util-keysyms-devel xcb-util-wm-devel libxkbcommon-devel libxkbcommon-x11-devel startup-notification-devel libxdg-basedir-devel xcb-util-xrm-devel cairo-gobject-devel pango-devel dbus-devel desktop-file-utils`

## Make

After these are installed, running `make package` should generate two rpms: `awesome` and `awesome-doc`. You'll want awesome-doc if you want a local copy of the API documentation. Helpful if on the go without internet.

## Mock

Fedora builds are commonly done with the `mock` package which is just a wrapper around `chroot`. This allows you to do work without polluting your system with the build packages. If you don't wish to install all the requirements you can run these commands:

```
# To make sure you have mock installed
dnf install mock

# After mock is installed, it should have installed all the mock config files
# Depending on what version you're running, choose your Fedora version and architecture
# For instance, 24 and x86_64

# Set up a mock environment
# This will set up a chroot in /var/lib/mock/fedora-24-x86_64/root/
mock -r fedora-24-x86_64 --init

# If you wish to edit files, install whatever editor you wish, here is a minimal version of vim
mock -r fedora-24-x86_64 --install vim-minimal

# Install all the required libraries (hostname is included in this list because it's not installed by default in a mock chroot)
mock -r fedora-24-x86_64 --install hostname git cmake lua-devel lua-lgi ImageMagick asciidoc xmlto gzip lua-ldoc libxcb-devel glib2-devel gdk-pixbuf2-devel cairo-devel xcb-util-cursor-devel xcb-util-devel xcb-util-keysyms-devel xcb-util-wm-devel libxkbcommon-devel libxkbcommon-x11-devel startup-notification-devel libxdg-basedir-devel xcb-util-xrm-devel cairo-gobject-devel pango-devel dbus-devel desktop-file-utils

# If you've already checked out the git repo (which you should have at this point, it's easier)
# then we'll cp in the entire git repo into the mock chroot tmp directory
cp -a /path/to/git/repo/awesome /var/lib/mock/fedora-24-x86_64/root/tmp/

# Now you should have the entire git tree within the /tmp directory in the mock chroot, we'll build out of there
# Request a shell into the mock chroot
mock -r fedora-24-x86_64 --shell

# You'll be sitting in the root dir /
# Go to your git tree, replacing repo with whatever you named your repo, assuming 'awesome'
cd /tmp/awesome

# Now make your rpms
make package

# To exit the mock chroot after you're done, or if you want to install other things (with the command above), just type:
exit

# After you're done with the mock chroot, you can delete it with:
mock -r fedora-24-x86_64 --clean
```

## Notes

The rpms generated follow the naming convention of `[package name][-component]-[version]-[release].[git string][dist]`. `[git string]` is in the format `YYYYMMDDgitHASH` where `YYYYMMDD` is the date of the latest commit and `HASH` is the short hash id for the latest commit. This allows future rpm builds of different dates to be recognized as newer without problem.

One problem with this is if your distro updates to a higher `[release]` than `1`, it will be recognized as newer and overrule all rpms created from the repo.

Another problem, albeit much less a problem than above, is creating multiple rpms with commits from the same day. `HASH` is not guaranteed to match as newer than another and could be seen as an older. At that point using `--force` to force an update is the only way until a newer commit with a future date comes in.
