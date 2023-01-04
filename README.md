# APEX Pipeline Simulator 
A template for 5 Stage APEX In-order Pipeline

## Notes:

 - This code is a simple implementation template of a working 5-Stage APEX In-order Pipeline
 - Implementation is in `C` language
 - Stages: Fetch -> Decode -> Execute -> Memory -> Writeback
 - All the stages have latency of one cycle
 - There is a single functional unit in Execute stage which perform all the arithmetic and logic operations
 - On fetching `HALT` instruction, fetch stage stop fetching new instructions
 - When `HALT` instruction is in commit stage, simulation stops

## Files:

 - `Makefile`
 - `file_parser.c` - Functions to parse input file
 - `apex_cpu.h` - Data structures declarations
 - `apex_cpu.c` - Implementation of APEX cpu
 - `apex_macros.h` - Macros used in the implementation
 - `main.c` - Main function which calls APEX CPU interface
 - `input.asm` - Sample input file

## How to compile and run

```
 Compile as follow
 make
```
 Run as follows:
```
[1] ./apex_sim input.asm single_step
->  prints each stage content on that clock cycle step by step (if you press any key it moves to next cycle, if you press q it stops)

[2] ./apex_sim input.asm simulate <number of clock cycles>
-> prints register file value and memory location values after specified clock cycle

[3] ./apex_sim input.asm show_mem <location>
-> prints memory location value specified on command

[4] ./apex_sim input.asm display <number of clock cycles>
-> prints every clock cycles's stage content till specified number
```

## Author

 - Rushi Patel (rpatel@binghamton.edu)
 - State University of New York, Binghamton
