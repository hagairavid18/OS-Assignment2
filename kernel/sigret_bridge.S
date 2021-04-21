
.globl sigret_call_start
.globl sigret_call_end

sigret_call_start:
  li a7, 24
  ecall
  ret              
sigret_call_end: