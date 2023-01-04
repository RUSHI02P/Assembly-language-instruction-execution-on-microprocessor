#include <stdio.h>
#include <stdlib.h>

#include "apex_cpu.h"

int
main(int argc, char const *argv[])
{
    APEX_CPU *cpu;

    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", VERSION);

    if (argc < 3) //argc != 2 
    {
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }

    
       cpu = APEX_cpu_init(argv[1]);
       if (!cpu)
       {
        fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
        exit(1);
       }  
    
    const char *str_1;
    if(argv[3])
    {
       str_1 = argv[3];
    }
    else
    {
       str_1 = "-1";
    }

    APEX_cpu_run(cpu, argv[2], str_1);
    APEX_cpu_stop(cpu);
    return 0;
}