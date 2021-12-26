#!/bin/sh

palette="palette.png"
video="video.mp4"
output="out.gif"

filters="fps=30,scale=256x192"
skip="-t 1 -ss 00:8"

# old
ffmpeg -i $video -vf "$filters,palettegen=max_colors=256:reserve_transparent=" -y $palette
ffmpeg -i $video -i $palette -lavfi "$filters [x]; [x][1:v] paletteuse=dither=none" -y $output

# unreleased
# ffmpeg -i $video -vf "$filters" out/out_%d.bmp

# audio
ffmpeg -i $video -f s16le -ac 1 -ar 22050 music.raw