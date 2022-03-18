#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <netdb.h>
#define row 6
#define column 7

//Used to establish the connection
struct addrinfo hints, *info, *p;
int serverSocket, clientSocket, secondSocket; //serverSocket can also be used to determine if the user is running the server or client
bool winState = false;
bool gameRunning = true;

//Function used to determine whether the program is running as client or server, and whether it is the user's turn or not
//(Makes the program more presentable
int determineCase(bool yourTurn) {
	if(serverSocket != 0) { //This is the server
		if(yourTurn) { //It is the server's turn
			return 1;
		}else { //It is not the server's turn
			return 2;
		}
	}else { //This is the client
		if(yourTurn) { //It is the client's turn
			return 3;
		}else { //It is not the client's turn
			return 4;
		}
	}
}

int getInput(bool yourTurn) {
	if(yourTurn) { //It's your turn to send an input
		bool valid = false;
		int input;
	
		while(!valid) {
			printf("\nEnter a column to drop a piece into (1-7)\nOR enter 10 to exit the game: ");
			scanf("\n%d", &input);
			if(input == 10) {
				printf("\nYou have chosen to end the game.\n");
				valid = true;
				gameRunning = false;
			}else if(input >= 1 && input <= 7) {
				valid = true;
				char export[2];
				sprintf(export, "%d", input);
				
				if(send(secondSocket, export, strlen(export), 0) < 0) {
					perror("Error while sending input\n");
					exit(1);
				}
				return input;
			}else {
				printf("\nNot a valid column, please try again.");
			}
		}
	}else { //It's not your turn, so you receive the other player's input
		printf("Waiting for the other player..\n");
		char import[1];
		if(recv(secondSocket, &import, sizeof(import), 0) < 0) {
			perror("Error while receiving other player's import\n");
			exit(1);
		}
		int output = atoi(import);
		return output;
	}
}

//Board-related functions
void setBoard(char *board) {
	for(int i = 0; i < row; i++) {
		for(int j = 0; j < column; j++) {
			*(board + i * column + j) = ' ';
		}
	}
}

void printBoard(char *board) {
	printf(" 1  2  3  4  5  6  7  \n");
	for(int i = 0; i < row; i++) {
		for(int j = 0; j < column; j++) {
			printf("[%c]", *(board + i * column + j));
		}
		printf("\n");
	}
	printf("\n");
}

void dropDisc(char *board, int targetColumn, bool yourTurn, int piecesPlaced) {
	int rowCheck = 5;
	
	if(*(board + (0 * column) + targetColumn) != ' ') {
		printBoard(board);
		printf("\nChosen column was full. Please choose again.\n");
		int input = getInput(yourTurn);
		dropDisc(board, input - 1, yourTurn, piecesPlaced);
	}else {
		while(*(board + rowCheck * column + targetColumn) != ' ' && rowCheck > 0) {
			rowCheck--;
		}
		switch(determineCase(yourTurn)) {
			case 1: //Running as Server, user's turn
				*(board + rowCheck * column + targetColumn) = 'X';
				piecesPlaced++;
				break;
			case 2: //Running as Server, not the user's turn
				*(board + rowCheck * column + targetColumn) = 'O';
				piecesPlaced++;
				break;
			case 3: //Running as Client, user's turn
				*(board + rowCheck * column + targetColumn) = 'O';
				piecesPlaced++;
				break;
			case 4: //Running as Client, not the user's turn
				*(board + rowCheck * column + targetColumn) = 'X';
				piecesPlaced++;
				break;
			default:
				break;
		}
	}
}

void makeConnection(int argc, char** argv) {
	if(argc < 3) {
		printf("Launching server..\n");
		char port[20];
		if(argc > 1) {
			strcpy(port, argv[1]);
		} else {
			strcpy(port, "7830");
			printf("No port found in arguments, defaulting to 7830\n");
		}
		memset(&hints, 0, sizeof(hints));

		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = PF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		getaddrinfo(NULL, port, &hints, &info);

		for(p = info; p != NULL; p->ai_next) {
			if((serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
				perror("Error creating the socket\n");
				exit(1);
			}
			if(bind(serverSocket, p->ai_addr, p->ai_addrlen) < 0) {
				perror("Binding error\n");
				close(serverSocket);
				exit(1);
			}
			break;
		}
		printf("Address: %s\n", inet_ntoa(((struct sockaddr_in *) info->ai_addr)->sin_addr));
		printf("Port:%s\n", port);
	
		freeaddrinfo(info);
		
		if(listen(serverSocket, 1) < 0) {
			perror("Listening error\n");
			exit(1);
		}
		
		struct sockaddr_storage clientAddrStorage;
		int size = sizeof(clientAddrStorage);
		if((secondSocket = accept(serverSocket, (struct sockaddr*) &clientAddrStorage, &size)) < 0) {
			perror("Unable to accept\n");
			exit(1);
		}
	}else if(argc == 3) {
		printf("Launching as client, connecting to\n");
		printf("Address: %s\nPort: %s\n", argv[1], argv[2]);
		memset(&hints, 0, sizeof(hints));
		
		hints.ai_family = PF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		
		getaddrinfo(argv[1], argv[2], &hints, &info);
		
		for(p = info; p != NULL; p->ai_next) {
			if((secondSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
				perror("Error with the socket\n");
				exit(1);
			}
			if(connect(secondSocket, p->ai_addr, p->ai_addrlen) < 0) {
				perror("Couldn't establish a connection\n");
				close(secondSocket);
				exit(1);
			}
			break;
		}
	}
}

void initialize(char name1[], char name2[], int argc, char** argv) {
	//Establishes the connection between server and client
	makeConnection(argc, argv);
	char myName[20], otherName[20];
	printf("Setting up the game..\n");

	//Checks whether the user is the server or client. When the user is the server, serverSocket will not equal 0
	if(serverSocket != 0) {
		printf("Hello. As the server you will be Player 1. Please enter your name:");
		scanf("%s", myName);
		printf("Waiting for Player 2..\n");
		//Error-checking
		if(send(secondSocket, myName, strlen(myName), 0) < 0) {
			perror("Error while sending Player 1 name\n");
			exit(1);
		}
		if(recv(secondSocket, &otherName, sizeof(otherName), 0) < 0) {
			perror("Error receiving Player 2 name\n");
			exit(1);
		}
		//Sets names
		strcpy(name1, myName);
		strcpy(name2, otherName);
	}else {
		printf("Hello. As the client, you will be Player 2. Please enter your name:");
		scanf("%s", myName);
		printf("Waiting for Player 1..\n");
		//Error-checking
		if(send(secondSocket, myName, strlen(myName), 0) < 0) {
			perror("Error while sending Player 2 name\n");
			exit(1);
		}
		if(recv(secondSocket, &otherName, sizeof(otherName), 0) < 0) {
			perror("Error receiving Player 1 name\n");
			exit(1);
		}
		//Sets names
		strcpy(name1, otherName);
		strcpy(name2, myName);
	}
	printf("\nPlayer 1 (%s) will be represented by X's\nPlayer 2 (%s) will be represented by O's\n", name1, name2);
}

void teardown(char *board) {
	printf("Destroying the game..\n");
	free(board);
}

bool checkWinState(char *board) {
	//Horizontal
	for(int i = 0; i < row; i++) {
		for(int j = 0; j < column-3; j++) {
			if(*(board + i * column + j) != ' ' && *(board + i * column + j) == *(board + i * column + j+1) && *(board + i * column + j) == *(board + i * column + j+2) && *(board + i * column + j) == *(board + i * column + j+3)) {
				printf("\nHorizontal win\n");
				return true;
			}
		}
	}

	//Vertical
	for(int i = 0; i < row - 3; i++) {
		for(int j = 0; j < column; j++) {
			if(*(board + i * column + j) != ' ' && *(board + i * column + j) == *(board + (i+1) * column + j) && *(board + i * column + j) == *(board + (i+2) * column + j) && *(board + i * column + j) == *(board + (i+3) * column + j)) {
				printf("\nVertical win\n");
				return true;
			}
		}
	}

	//Diagonal
	//Top left to bottom right ("Negative slope")
	for(int i = 0; i < row - 3; i++) {
		for(int j = 0; j < column - 3; j++) {
			if(*(board + i * column + j) != ' ' && *(board + i * column + j) == *(board + (i+1) * column + j + 1) && *(board + i * column + j) == *(board + (i+2) * column + j + 2) && *(board + i * column + j) == *(board + (i+3) * column + j + 3)) {
				printf("\nDiagonal win\n");
				return true;
			}
		}
	}

	//Bottom left to top right ("Positive slope")
	for(int i = 0; i < row - 3; i++) {
		for(int j = 0; j < column - 3; j++) {
			if(*(board + i * column + j) != ' ' && *(board + i * column + j) == *(board + (i+1) * column + j - 1) && *(board + i * column + j) == *(board + (i+2) * column + j - 2) && *(board + i * column + j) == *(board + (i+3) * column + j - 3)) {
				printf("\nDiagonal win\n");
				return true;
			}
		}
	}
	
	//If none of the requirements have been met
	return false;
}

void displayWorldState(char name1[], char name2[], char *board, bool yourTurn) {
	if(!winState) {
		switch(determineCase(yourTurn)) {
			case 1: //Running as Server, user's turn
				printf("\nIt's %s's turn\n", name1);
				break;
			case 2: //Running as Server, not the user's turn
				printf("\nIt's %s's turn\n", name2);
				break;
			case 3: //Running as Client, user's turn
				printf("\nIt's %s's turn\n", name2);
				break;
			case 4: //Running as Client, not the user's turn
				printf("\nIt's %s's turn\n", name1);
				break;
			default:
				break;
		}
		printBoard(board);
	}else {
		printBoard(board);
		switch(determineCase(yourTurn)) {
			case 1: //Running as Server, user's turn
				printf("\nCongratulations, %s wins!\n", name1);
				break;
			case 2: //Running as Server, not the user's turn
				printf("\nCongratulations, %s wins!\n", name2);
				break;
			case 3: //Running as Client, user's turn
				printf("\nCongratulations, %s wins!\n", name2);
				break;
			case 4: //Running as Client, not the user's turn
				printf("\nCongratulations, %s wins!\n", name1);
				break;
			default:
				break;
		}
	}
	
}

void updateWorldState(char *board, int boardIndex, bool yourTurn, int piecesPlaced) {
	dropDisc(board, boardIndex, yourTurn, piecesPlaced);
	
	winState = checkWinState(board);
	if(winState) {
		gameRunning = false;
	}

	if(piecesPlaced == 42) {
		gameRunning = false;
	}
}

int main(int argc, char** argv) {
	//Launch as server/host: ./ConnectFour3
	//Launch as client: ./ConnectFour3 [host's address] [host's port number]
	char name1[20], name2[20];
	bool yourTurn;
	int piecesPlaced = 0;
	
	initialize(name1, name2, argc, argv);

	//Board is a pointer to accomodate for dynamic memory allocation
	char *board = (char*)malloc(row * column * sizeof(char));
	setBoard(board);
	
	if(serverSocket != 0) {
		yourTurn = true;
	}else {
		yourTurn = false;
	}
	
	int input;
	displayWorldState(name1, name2, board, yourTurn);
	do {
		input = getInput(yourTurn);
		if(input != 10) { //10 signifies that a player wants to end the game
			updateWorldState(board, input - 1, yourTurn, piecesPlaced);
			if(gameRunning) {
				yourTurn = !yourTurn;
			}
			displayWorldState(name1, name2, board, yourTurn);
		}
	}while(gameRunning);
	if(piecesPlaced == 42 && !winState) {
		printf("\nThe board is full!\n");
	}

	teardown(board);
	return 0;
}