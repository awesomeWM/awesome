#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, const char **argv) {
    // Parse args
    const int winX = atoi(argv[1]);
    const int winY = atoi(argv[2]);
    const int winW = atoi(argv[3]);
    const int winH = atoi(argv[4]);

    // Open display
    Display *dpy = XOpenDisplay(NULL);
    if(!dpy) return 1;
    int scr = DefaultScreen(dpy);

    // Create window
    XVisualInfo vinfo;
    XMatchVisualInfo(dpy, scr, 32, TrueColor, &vinfo);
    Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, scr), vinfo.visual, AllocNone);
    XSetWindowAttributes swa = {0};
    swa.colormap = cmap;
    swa.border_pixel = 0;
    Window win = XCreateWindow(
        dpy, RootWindow(dpy, scr),
        winX, winY, winW, winH, 0,
        vinfo.depth, InputOutput, vinfo.visual,
        CWColormap | CWBorderPixel,
        &swa
    );
    XSelectInput(dpy, win, ButtonPressMask | ExposureMask);
    XStoreName(dpy, win, "testInputShape");
    XGCValues gcv;
    GC gc = XCreateGC(dpy, win, 0, &gcv);
    XMapWindow(dpy, win);

    // Set right half of the image to be clickable
    XRectangle rectHalf = {
        winW/2,
        0,
        winW/2,
        winH
    };
    XShapeCombineRectangles(
        dpy,
        win,
        ShapeInput,      /* destination_kind */
        0, 0,            /* offsets */
        &rectHalf, 1,    /* rectangles */
        ShapeSet,        /* operation */
        YXBanded         /* ordering */
    );

    // Main loop
    printf("Ready\n");
    fflush(stdout);
    XImage *img = XCreateImage(dpy, vinfo.visual, 32, ZPixmap, 0, malloc(winW*winH*4), winW,winH, 32, winW*4);
    while(1) {
        XEvent e;
        while(XPending(dpy)) {
            XNextEvent(dpy, &e);
            if(e.type == ButtonPress) {
                printf("Click %d %d\n", e.xbutton.x, e.xbutton.y);
                fflush(stdout);
            }
        }

        // Render
        const static unsigned char colorRed[4] = { 0, 0, 128, 128};
        const static unsigned char colorGreen[4] = { 0, 128, 0, 128};
        unsigned char *p = (unsigned char*)img->data;
        for(int y=0; y < winH; y++) {
            for(int x=0; x < winW; x++) {
                const unsigned char *srcColor = x >= winW/2 ? colorGreen : colorRed;
                unsigned char *dstColor = p + 4 * (y*winW + x);
                memcpy(dstColor, srcColor, 4);
            }
        }
        XPutImage(dpy,win,gc,img,0,0,0,0,winW,winH);

        // Wait for 100ms
        usleep(100000);
    }
}