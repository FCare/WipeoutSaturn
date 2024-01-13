#!/bin/sh

./build/convertX3D ../blender/results/solaar.x3d ../blender/results/solaar_end.png
./build/convertX3D ../blender/results/anasta.x3d ../blender/results/anasta\ end.png
./build/convertX3D ../blender/results/arial.x3d ../blender/results/arial\ end.png
./build/convertX3D ../blender/results/arian.x3d ../blender/results/arian_end.png
./build/convertX3D ../blender/results/chang.x3d ../blender/results/chang\ end.png
./build/convertX3D ../blender/results/dekka.x3d ../blender/results/dekka_end.png
./build/convertX3D ../blender/results/jacko.x3d ../blender/results/jacko\ end.png
./build/convertX3D ../blender/results/sophia.x3d ../blender/results/sophia_end.png

./build/convertX3D ../blender/results/col_solaar.x3d
./build/convertX3D ../blender/results/col_anasta.x3d
./build/convertX3D ../blender/results/col_arial.x3d
./build/convertX3D ../blender/results/col_arian.x3d
./build/convertX3D ../blender/results/col_chang.x3d
./build/convertX3D ../blender/results/col_dekka.x3d
./build/convertX3D ../blender/results/col_jacko.x3d
./build/convertX3D ../blender/results/col_sophia.x3d

cp ../blender/results/*.smf ../../cd/wipeout/ship/