# RUN: llvm-mc -filetype=obj -triple riscv64 -mattr=+c %s | llvm-objdump -d -M no-aliases --no-show-raw-insn - | FileCheck %s --check-prefix=INSTR

# RUN: llvm-mc -filetype=obj -triple riscv64 -mattr=+c %s --mc-relax-all | llvm-objdump -d -M no-aliases --no-show-raw-insn - | FileCheck %s --check-prefix=RELAX-INSTR

## Check the instructions are relaxed correctly

NEAR:

# INSTR:           c.beqz    a0, 0x0 <NEAR>
# RELAX-INSTR:     c.bnez    a0, 0x6
# RELAX-INSTR-NEXT:jal       zero, 0x0 <NEAR>
c.beqz a0, NEAR

# INSTR:           c.j    0x0 <NEAR>
# RELAX-INSTR:     jal    zero, 0x0 <NEAR>
c.j NEAR

bnez s0, .foo
j    .foo
beqz s0, .foo
.foo:
ret
