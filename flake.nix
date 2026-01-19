{
  description = "libdatachannel development environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            cmake
            gcc
            pkg-config
            openssl
            (python3.withPackages (
              ps: with ps; [
                websockets
              ]
            ))

            # Video/Audio data composition framework tools like "gst-inspect", "gst-launch" ...
            gst_all_1.gstreamer
            # Common plugins like "filesrc" to combine within e.g. gst-launch
            gst_all_1.gst-plugins-base
            # Specialized plugins separated by quality
            gst_all_1.gst-plugins-good
            gst_all_1.gst-plugins-bad
            gst_all_1.gst-plugins-ugly
            # Plugins to reuse ffmpeg to play almost every video format
            gst_all_1.gst-libav
            # Support the Video Audio (Hardware) Acceleration API
            gst_all_1.gst-vaapi
          ];
        };
      }
    );
}
