#!/bin/bash
gcc -c main.c -o main.o
gcc -c tokenizer.c -o tokenizer.o
gcc -c AST.c -o AST.o
gcc -c codeGen.c -o codeGen.o
gcc -c hashmapop.c -o hashmapop.o
gcc main.o tokenizer.o AST.o codeGen.o hashmapop.o -o compiler

