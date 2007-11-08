# awesome version
VERSION = $$(git describe 2>/dev/null || echo devel)
RELEASE = "Productivity Breaker"

# Customize below to fit your system

# additional layouts
LAYOUTS = layouts/tile.c layouts/floating.c layouts/max.c

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/include/X11
X11LIB = /usr/lib/X11

# includes and libs
INCS = -I. -I/usr/include -I${X11INC} `pkg-config --cflags libconfuse xft cairo`
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 `pkg-config --libs libconfuse xft cairo` -lXext -lXrandr -lXinerama

# flags
CFLAGS = -fgnu89-inline -std=gnu99 -ggdb3 -pipe -Wall -Wextra -W -Wchar-subscripts -Wundef -Wshadow -Wcast-align -Wwrite-strings -Wsign-compare -Wunused -Wuninitialized -Winit-self -Wpointer-arith -Wredundant-decls -Wno-format-zero-length -Wmissing-prototypes -Wmissing-format-attribute -Wmissing-noreturn -O3 ${INCS} -DVERSION=\"${VERSION}\" -DRELEASE=\"${RELEASE}\"
LDFLAGS = -ggdb3 ${LIBS}

# compiler and linker
CC = cc
