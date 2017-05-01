#!/usr/bin/env bash

if [ ! -d "$HOME/.cathook" ]; then
  mkdir "$HOME"/.cathook
fi

if [ ! -f "$HOME/.cathook/killsays.txt" ]; then
  cp res/killsays.txt "$HOME"/.cathook
fi

if [ ! -f "$HOME/.cathook/spam.txt" ]; then
  cp res/spam.txt "$HOME"/.cathook
fi

echo "Default killsay/spam files installed, EDIT THEM!"
xdg-open "$HOME"/.cathook
