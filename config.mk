# awesome version
VERSION = devel

# Customize below to fit your system

# additional layouts beside floating
LAYOUTS = layouts/tile.c layouts/floating.c layouts/max.c

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/include/X11
X11LIB = /usr/lib/X11

# includes and libs
INCS = -I. -I/usr/include -I${X11INC} `pkg-config --cflags libconfig xft`
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 `pkg-config --libs libconfig xft` -lXext -lXrandr -lXinerama

# flags
CFLAGS = -fgnu89-inline -std=gnu99 -ggdb3 -pipe -Wall -Wextra -W -Wchar-subscripts -Wundef -Wshadow -Wcast-align -Wwrite-strings -Wsign-compare -Wunused -Wuninitialized -Winit-self -Wpointer-arith -Wredundant-decls -Wno-format-zero-length -Wmissing-prototypes -Wmissing-format-attribute -Wmissing-noreturn -O3 ${INCS} -DVERSION=\"${VERSION}\"
LDFLAGS = -ggdb3 ${LIBS}

# compiler and linker
CC = cc
