#define _CRT_SECURE_NO_WARNINGS

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include <stdio.h>
#include <stdlib.h>

#include "include/hospital.h"

int main(void) {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    ensureDataDirectory();
    return runWebServer(8080);
}
