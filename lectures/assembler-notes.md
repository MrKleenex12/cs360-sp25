# Assembly Notes

## Architecture

We are assuming a general computer architecture that consists of:

### Registers

We will assume that our machine has 8 general-purpose registers in the CPU. All are 4 bytes and can be read or written by the user.  
The first 5 are **r0, r1, r2, r3, and r4**.

The last three are special:

- **sp** - stack pointer
- **fp** - frame pointer
- **pc** - program counter

There are three read-only registers, which always contain the same values:

- **g0** - 0
- **g1** - 1
- **gm1** - -1

There are two special registers that the user cannot access directly

- **IR** - Instruction register. It holds the instruction currently being executed
- **CSR** - The control status register. It contains information pertaining to the execution of the current and previous instructions
