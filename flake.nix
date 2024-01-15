{
  description = "A template for Nix based C++ project setup.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, ... }@inputs: inputs.utils.lib.eachSystem [
    # Add the system/architecture you would like to support here. Note that not
    # all packages in the official nixpkgs support all platforms.
    "x86_64-linux" "i686-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin"
  ] (system: let
    pkgs = import nixpkgs {
      inherit system;
    };
  in {
    devShells.default = pkgs.mkShell rec {
      # Update the name to something that suites your project.
      name = "wikidice";
      stdenv = pkgs.llvmPackages_17.libcxxStdenv;
      packages = with pkgs; [
        # Development Tools
        cmake
        ninja
        extra-cmake-modules
        pkg-config
        llvmPackages_17.clang
  
        # Development time dependencies
        cppcheck

        # Build time and Run time dependencies
        zstd
      ];

      # Setting up the environment variables you need during
      # development.
      shellHook = let
        icon = "f121";
      in ''
        export PS1="$(echo -e '\u${icon}') {\[$(tput sgr0)\]\[\033[38;5;228m\]\w\[$(tput sgr0)\]\[\033[38;5;15m\]} (${name}) \\$ \[$(tput sgr0)\]"
      '';
    };
    packages.default = pkgs.callPackage ./default.nix {};
  });
}
