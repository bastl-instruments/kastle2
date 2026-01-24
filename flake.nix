{
  description = "Kastle 2 development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs, ... }:
    let
      # Helper to generate a dev shell output for all supported architecture:systems pairings
      forAllSystems =
        function:
        nixpkgs.lib.genAttrs [
          "aarch64-darwin"
          "aarch64-linux"
          "x86_64-darwin"
          "x86_64-linux"
        ] (system: function nixpkgs.legacyPackages.${system});
    in
    {
      devShells = forAllSystems (
        pkgs:
        let
          pico-sdk = pkgs.pico-sdk.override { withSubmodules = true; };
        in
        {
          default = pkgs.mkShell {
            name = "kastle-2-dev-shell";

            buildInputs = [
              pkgs.cmake
              pkgs.doxygen
              pkgs.gcc-arm-embedded
              pkgs.gnumake
              pkgs.openocd
              pkgs.picotool
              pkgs.pkg-config
              pkgs.python3
            ];

            PICO_SDK_PATH = "${pico-sdk}/lib/pico-sdk";
            PICO_TOOLCHAIN_PATH = "${pkgs.gcc-arm-embedded}/bin";

            shellHook =
              let
                gum = "${pkgs.gum}/bin/gum";
              in
              ''
                if [ ! -f "./code/build/Makefile" ]; then
                  echo "No ./code/build/Makefile file, invoking ./configure.sh"
                  ./configure.sh
                fi

                TITLE=$(${pkgs.gum}/bin/gum style \
                  --bold \
                  --foreground '#00fc94' \
                  "🏰 Bastl Kastle 2 Development Shell 🏰")

                SUBTITLE=$(${pkgs.gum}/bin/gum style \
                  --align center \
                  --width 40 \
                  "Ready for development")

                ${pkgs.gum}/bin/gum style \
                  --align center \
                  --border thick \
                  --padding "1 1" \
                  --border-foreground '#cdb8a5' \
                  "$TITLE

                  $SUBTITLE"
              '';
          };
        }
      );
    };
}
