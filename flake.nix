{
  description = "Syzyf Engine - C++, OpenGL";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};

      runtimeLibs = with pkgs; [
        libGL
        libGLU

        wayland
        libxkbcommon
        libxinerama
        xorg.libX11
        xorg.libXcursor
        xorg.libXi
        xorg.libXrandr
        xorg.libXext
      ];
    in
    {
      devShells.${system}.default = pkgs.mkShell {

        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          ccache
          pkg-config
          gdb
          wayland-scanner

          glslls
          clang-tools
        ];

        buildInputs = with pkgs; [
          zlib
          libffi
        ] ++ runtimeLibs;

        shellHook = ''
          export LD_LIBRARY_PATH=${pkgs.lib.makeLibraryPath runtimeLibs}:$LD_LIBRARY_PATH
        '';
      };
    };
}
