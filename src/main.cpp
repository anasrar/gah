#include <iostream>
#ifdef __linux__
  #include <stdlib.h>
#endif
#include "gah.h"

int main(int argc, char const *argv[])
{
  #ifdef __linux__
    setenv("LC_ALL", "C", 1);
  #endif
	GAH::db_connect();
	GAH::main_actions(argc, argv);
	GAH::db_disconnect();
	return 0;
}
