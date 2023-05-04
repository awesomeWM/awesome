{
    description = "AwesomeWM Flake";

    inputs = {
        nixpkgs.url = github:NixOS/nixpkgs/nixpkgs-unstable;
        flake-utils.url = github:numtide/flake-utils;
    };

    outputs = {
        self
        , nixpkgs
        , ...
    }@inputs:
    inputs.flake-utils.lib.eachDefaultSystem (system:
    let
        pkgs = nixpkgs.legacyPackages.${system};
    in
    {
        devShell = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
                (lua.withPackages(p: with p; [
                    busted
                    lgi
                    luacheck
                ]))
                (with xorg; [
                    libpthreadstubs
                    libXau
                    libXdmcp
                    libxcb
                    libxshmfence
                    xcbutil
                    xcbutilimage
                    xcbutilkeysyms
                    xcbutilrenderutil
                    xcbutilwm
                ])
                libxkbcommon
                xcbutilxrm
                xcb-util-cursor
                pango
                cairo
                librsvg
                gdk-pixbuf
                dbus
                gobject-introspection
                libstartup_notification
                libxdg_basedir
                nettools
                cmake

                # doc/dev deps
                doxygen
                imagemagick
                pkg-config
                xmlto
                docbook_xml_dtd_45
                docbook_xsl
                findXMLCatalogs
                asciidoctor
            ];
        };
    });
}

#vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
