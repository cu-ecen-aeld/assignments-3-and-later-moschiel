#!/bin/sh

make
valgrind --leak-check=yes ./aesdsocket