{ pkgs ? import <nixpkgs> {} }:
let
  #
in pkgs.mkShell {
    buildInputs = with pkgs; [
        ncurses
    ];
}
