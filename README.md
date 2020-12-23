# 3D-Checkers

CPE 471 Final Project

A game of 3D-Checkers written using C++

Pressing the 'a' and 'd' keys will rotate the camera clockwise and 
coutner-clockwise respectively around the game board. Pressing the 'w' and 's' 
keys will move the camers up and down respectively. The mouse can be used to 
rotate the camera. By clicking the left mouse button and dragging the mouse 
around the scene, the camera will rotate in the direction of the mouse. The 
camera is bound to only be able to look down below the horizontal. It is also 
bound to not be able to go below the ground. Pressing the 'r' key will reset 
the entire scene to its original state, with all pieces in their original 
positions and the camera in its original location above player 1. Pressing the 
'k' key when having the selecting tile over one of your own pieces will make it 
a king if it is not, and will remove the king if it already is one. The camera 
position is used as the light source, so as the camera moves, so does the 
light. The arrow keys can be used to move the selecting tile up, down, left, 
and right, with respect to the current player's starting view. When selecting 
a piece to move, only pieces that can have a valid location to move to can be 
selected. Valid selections color the selecting tile green, and invalid options 
color the tile red. When choosing a tile to move a piece to, again, only valid 
tiles can be choosen; valid tiles appear in green when the selecting tile is 
on them, and invalid tiles appear red. Pieces can only move according to legal 
checker rules, except double jumps. There are no double jumps. PLayers can 
only select pieces of their color. When a player's turn ends, the camera 
automatically shifts to the other player's side of the board. When a piece is 
jumped, it spawns an explosion of particles that is the same color as the 
piece that was jumped. When a king becomes a king legally, it spawns an 
explosion of golden particles at its position. Textures are used on the 
benches and the dogs on top of the rocks at the corners of the game board. 
There is also a textured skybox to give the scene a forest feel. The game 
pieces are hierarchically modeled and are also animated. The fans on the 
benches are also hierarchically modeled and animated in similar ways.