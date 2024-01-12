#!/bin/sh

./build/convertX3D ../blender/results/solaar.x3d ../blender/results/solaar_end.png
./build/convertX3D ../blender/results/anasta.x3d ../blender/results/anasta\ end.png
./build/convertX3D ../blender/results/arial.x3d ../blender/results/arial\ end.png
./build/convertX3D ../blender/results/arian.x3d ../blender/results/arian_end.png
./build/convertX3D ../blender/results/chang.x3d ../blender/results/chang\ end.png
./build/convertX3D ../blender/results/dekka.x3d ../blender/results/dekka_end.png
./build/convertX3D ../blender/results/jacko.x3d ../blender/results/jacko\ end.png
./build/convertX3D ../blender/results/sophia.x3d ../blender/results/sophia_end.png

cp ../blender/results/*.smf ../../cd/wipeout/ship/