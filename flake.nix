{
  description = "AwesomeWM";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }: {
    overlay = final: prev: {
      awesome = (prev.awesome.overrideAttrs (old: {
        version = "latest";
        src = ./.;

        patches = [ ];

        cmakeFlags = old.cmakeFlags ++ [
          "-DGENERATE_MANPAGES=OFF"
          "-DGENERATE_DOC=OFF"
        ];
      }));
    };
  } // flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs {
        overlays = [ self.overlay ];
        inherit system;
      };
    in
    {
      packages = with pkgs; {
        default = awesome;
        inherit awesome;
      };
    });
}

