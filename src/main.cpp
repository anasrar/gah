#include <iostream>
#include "gah.h"

int main(int argc, char const *argv[])
{
	GAH::db_connect();
	GAH::main_actions(argc, argv);
	GAH::db_disconnect();
	return 0;
}
