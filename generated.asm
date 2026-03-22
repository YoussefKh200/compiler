section .data
  printf_format: db '%d', 10, 0
extern printf
global main
section .text
main:
  mov rax, 12
  push rax
  mov rax, 60
  pop rdi
  syscall
