### Installing current git master as a package receipts

#### Arch Linux AUR

```

sudo pacman -S --needed base-devel git
git clone https://aur.archlinux.org/awesome-git.git
cd awesome-git
makepkg -fsri

```

#### Debian-based

```

sudo apt build-dep awesome
git clone https://github.com/awesomewm/awesome
cd awesome
make package
sudo apt install *.deb

```

#### NixOS (overlay)

**NOTE**: use `nix-prefetch-github` every time when you want to update also be aware that build may fail when dependencies will be changed.

```

cd /etc/nixos
touch source.json
nix-shell -p nix-prefetch-github --run 'nix-prefetch-github awesomeWM awesome --json >> source.json'

```


```nix

# Put this overlay in your configuration.nix above X11 section

  nixpkgs.overlays = let
    source = builtins.fromJSON (builtins.readFile ./source.json);
  in [
    (self: super: {
      awesome = super.awesome.overrideAttrs (old: {
        src = super.fetchFromGitHub rec {
          name = "source-${owner}-${repo}-${rev}";
          inherit (source) owner repo rev sha256;
        };
      });
    })
  ];

```