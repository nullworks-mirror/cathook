if [ ! -d "~/.cathook" ]; then
	mkdir ~/.cathook
fi

if [ ! -f "~/.cathook/killsays.txt" ]; then
	cp res/killsays.txt ~/.cathook
fi

if [ ! -f "~/.cathook/spam.txt" ]; then
	cp res/spam.txt ~/.cathook
fi

echo Default killsay/spam files installed, EDIT THEM!
gnome-open ~/.cathook