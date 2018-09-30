with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "meowmeow-devenv";
  buildInputs = with xorg; [ libX11 xinit xorgserver ];
}
