# NixOS

<div align="center">

# Running and installing on NixOS

This guide explains how to run and install **BeeLauncher** on NixOS.

</div>

## Running without installation

```fish
nix run github:Sudo-Ivan/BeeLauncher#beelauncher
```

## Installation

Add the flake input to your `flake.nix`:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    beelauncher = {
      url = "github:Sudo-Ivan/BeeLauncher";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, beelauncher, ... }: {
    # your outputs
  };
}
```

### NixOS configuration

```nix
{ pkgs, system, beelauncher, ... }:

{
  environment.systemPackages = [
    beelauncher.packages.${system}.beelauncher
  ];
}
```

### Home Manager configuration

```nix
{ pkgs, system, beelauncher, ... }:

{
  home.packages = [
    beelauncher.packages.${system}.beelauncher
  ];
}
```

## Updating

To update the flake input:

```fish
nix flake update beelauncher
```

Or update all inputs:

```fish
nix flake update
```
