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
        libxcb
        libxtst
        dbus

        alsa-lib
        libpulseaudio
        pipewire
      ];
    in
    {
      devShells.${system}.default = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {

        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          ccache
          pkg-config
          gdb
          wayland-scanner
          python3
          tracy-wayland
          zenity

          cppcheck
          include-what-you-use
          glslang
          glslls
          clang-tools
          doxygen
          graphviz
        ];

        buildInputs = with pkgs; [
          zlib
          libffi
          libclang
        ] ++ runtimeLibs;

        shellHook = ''
          export LD_LIBRARY_PATH=${pkgs.lib.makeLibraryPath runtimeLibs}:$LD_LIBRARY_PATH

          export CPLUS_INCLUDE_PATH=$(clang++ -E -x c++ - -v < /dev/null 2>&1 | awk '/#include <...>/ {flag=1; next} /End of search list/ {flag=0} flag {print $1}' | tr '\n' ':' | sed 's/:$//')

          export LIBCLANG_LIBRARY_PATH="${pkgs.lib.getLib pkgs.libclang}/lib"
        '';
      };
    };
}

