{
  description = "Prism Launcher fork aimed to provide a free way to play Minecraft.";

  nixConfig = {
    substituters = [
      "https://cache.nixos.org"
      "https://cache.garnix.io"
    ];
    trusted-public-keys = [
      "cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY="
      "cache.garnix.io:CTFPyKSLcx5RMJKfLo5EEPUObbA78b0YQ2DTCJXqr9g="
    ];
  };

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nix-filter.url = "github:numtide/nix-filter";
    libnbtplusplus = {
      url = "github:FreesmTeam/libnbtplusplus";
      flake = false;
    };
  };

  outputs = {
    self,
    nixpkgs,
    nix-filter,
    libnbtplusplus,
    ...
  }: let
    systems = [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ];

    forEachSystem = nixpkgs.lib.genAttrs systems;

    mkJvmPack = pkgs: let
      openjdk = with pkgs; [
        openjdk8
        openjdk17
        openjdk21
        openjdk25
      ];

      temurin = with pkgs.javaPackages.compiler.temurin-bin; [
        jdk-8
        jdk-17
        jdk-21
        jdk-25
      ];

      corretto = with pkgs.javaPackages.compiler; [
        corretto17
        corretto21
        corretto25
      ];

      graal-ce = with pkgs.graalvmPackages; [
        graalvm-ce
      ];

      graal-unfree = with pkgs.graalvmPackages; [
        graalvm-oracle_17
        graalvm-oracle_25
      ];
    in {
      inherit openjdk temurin corretto graal-ce graal-unfree;
      allPack = openjdk ++ temurin ++ corretto ++ graal-ce ++ graal-unfree;
    };
  in {
    overlays.default = final: prev: {
      beelauncher-unwrapped = final.callPackage ./nix/unwrapped.nix {
        inherit nix-filter libnbtplusplus self;
      };

      beelauncher = final.callPackage ./nix/wrapper.nix {
        jvmPack = mkJvmPack final;
      };
    };

    packages = forEachSystem (system: let
      pkgs = import nixpkgs {inherit system;};

      jvmPack = mkJvmPack pkgs;

      beelauncher-unwrapped = pkgs.callPackage ./nix/unwrapped.nix {
        inherit nix-filter libnbtplusplus self;
      };

      beelauncher = pkgs.callPackage ./nix/wrapper.nix {
        inherit beelauncher-unwrapped jvmPack;
      };

      beelauncher-unwrapped-debug = beelauncher-unwrapped.overrideAttrs {
        cmakeBuildType = "Debug";
        dontStrip = true;
      };

      beelauncher-debug = pkgs.callPackage ./nix/wrapper.nix {
        beelauncher-unwrapped = beelauncher-unwrapped-debug;
      };
    in {
      inherit beelauncher beelauncher-unwrapped beelauncher-debug beelauncher-unwrapped-debug jvmPack;

      default = beelauncher;
    });

    devShells = forEachSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
        overlays = [self.overlays.default];
      };
    in {
      default = pkgs.mkShell {
        inputsFrom = [pkgs.beelauncher-unwrapped];

        packages = with pkgs; [
          ccache
          ninja
        ];
      };
    });

    formatter = forEachSystem (
      system:
        (import nixpkgs {inherit system;}).alejandra
    );
  };
}
