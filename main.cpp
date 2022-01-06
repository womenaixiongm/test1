#include <iostream>
#include "application_interface.h"


int main(int argc, char **argv) {
    ApplicationInterface application;
    application.Start(argc, argv);
    return 0;
}
