# Connect-Four
Program to run a two-player game of Connect Four, written in C, an iterative project for ICSI 333 (Programming at the Hardware/Software Interface).
This is a program to run a two-player game of Connect Four, written in C.
An iterative project for ICSI 333 (Programming at the Hardware/Software Interface) in Fall 2020.

Tested in Windows command line and compiled using GCC, the program can be run as a Server/Host or Client on a single machine.
Running the program without any arguments will default the server's address and port number to 0.0.0.0 and 7830, respectfully.
Running the program with the host's address and port number will run the program as a client, connecting to the server
(i.e. './ConnectFour 0.0.0.0 7830')

Due to the limitations presented by firewall and other measures, the program is only designed to run on a single machine through two instances of the command line.
However, these two instances will communicate and agree on whose turn it currently is and updating the board after each player's move, detecting 
when a someone makes a winning move and stopping the game.
