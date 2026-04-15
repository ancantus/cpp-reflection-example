#!/bin/sh
set -x

/opt/gcc-trunk/bin/g++ -std=c++26 -freflection enum.cpp -o enum
/opt/gcc-trunk/bin/g++ -std=c++26 -freflection pack_test.cpp -o pack
