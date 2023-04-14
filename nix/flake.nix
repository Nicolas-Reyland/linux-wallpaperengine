{
  description = "A flake for building linux-wallpaperengine";

  inputs.nixpkgs.url = github:NixOS/nixpkgs/nixos-22.05;

  outputs = { self, nixpkgs }: {
    with import nixpkgs { system = "x86_64-linux" };
    stdenv.mkDerivation {
      name = "linux-wallpaperengine";
      src = self;
      buildPhase = "cmake -B build && cmake --build build --target linux-wallpaperengine"
      installPhase = "";
    };
  };
}
