clear
clear
emcc menger.c -lidbfs.js -O3 -s USE_SDL=2 -s ENVIRONMENT=web -o bin/index.html --shell-file t.html

