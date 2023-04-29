#include <iostream>

#include "Server.h"

int main() {
	SERVER::Server server = SERVER::Server(9909, 5);
	server.start();
}
