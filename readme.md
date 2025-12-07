# Chipz

Chipz is a toy Chip-8 emulator. It is not designed to be used by anyone, nor as reference by anyone.  
Chipz precomputes all possible arguments that can be passed into a function at compile time, then templates them in. And
stores a function pointer to this compiled version in a jump table. This is not the best way to do this. Far from it.
To change the rom you must recompile the program after adding it to `host.cpp` and setting the loader to use it.

# WARNING

DO NOT USE THIS EMULATOR AS REFERENCE

## Performance

Chipz currently hits a very low 3 mipf (Million instructions per frame), on an AMD Ryzen 7 5700x3D.