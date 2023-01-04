/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* 
Converts the PC(4000 series) into array index for code memory
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        {
             printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
             break;
        }
        case OPCODE_ADDL:
        {
             printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
             break;
        }
        case OPCODE_SUB:
        {
             printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
             break;
        }
        case OPCODE_SUBL:
        {
             printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
             break;
        }
        case OPCODE_MUL:
        {
             printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
             break;
        }
        case OPCODE_DIV:
        {
             printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
             break;
        }
        case OPCODE_AND:
        {
             printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
             break;
        }
        case OPCODE_OR:
        {
             printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
             break;
        }
        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_LDR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_STORE:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_STR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rs3, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_BZ:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
            break;
        }

        case OPCODE_NOP:
        {
            printf("%s ", stage->opcode_str);
            break;
        }

        case OPCODE_BNZ:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_HALT:
        {
            printf("%s", stage->opcode_str);
            break;
        }

    }
}

/* 
Debug function which prints the CPU stage content 
*/
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-40s pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* 
Debug function which prints the register file
 */
/*
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
}
*/


// flag value for 16 registers to check data dependancies
int flags[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
// helper variable to stop fetching new instruction at fetch stage 
int stall = 0;
//if command line argument == simulate then flag will be 1.
int command_simulate = 0;


// Print register file
static void print_reg_flag(const APEX_CPU *cpu)
{
   int i;

   printf("=============== STATE OF ARCHITECTURAL REGISTER FILE ============== \n");
   printf("If register's status is 0 then its valid else if 1 then invalid \n");

   for(i = 0; i < REG_FILE_SIZE; i++)
   {
     printf("| REG[%-2d] | Value = %-4d | Status = %d |", i, cpu->regs[i], flags[i]);
     printf("\n");
   }
}

//print 0 to 99 memory location
static void print_mem(const APEX_CPU *cpu)
{
   int i;

   printf("============== STATE OF DATA MEMORY ============= \n");

   for(i = 0; i < 100; i++)
   {
    printf("|   MEM[%-2d]   |   Data Value = %d   |", i, cpu->data_memory[i]);
    printf("\n");
   }
}



/*
Fetch Stage of APEX Pipeline
*/
static void
APEX_fetch(APEX_CPU *cpu)
{

    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn)
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        // if stall caused in fetch by decode stage, then skip cycle

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.rs3 = current_ins->rs3;
        cpu->fetch.imm = current_ins->imm;

        if(stall == 1){
            if (ENABLE_DEBUG_MESSAGES && command_simulate == 0) {
            print_stage_content("Instruction at FETCH_STAGE     --->", &cpu->fetch);
            }
            return;
        }

        /* Update PC for next instruction */
        cpu->pc += 4;

        /* Copy data from fetch latch to decode latch*/
        cpu->decode = cpu->fetch;
        
        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
            print_stage_content("Instruction at FETCH_STAGE     --->", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT)
        {
            cpu->fetch.has_insn = FALSE;
        }
    }
    else
    {
        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
           printf("Instruction at FETCH_STAGE     --->      EMPTY \n");
        }
       
    }
}

/*
Decode Stage of APEX Pipeline
*/
static void
APEX_decode(APEX_CPU *cpu)
{
        if (cpu->decode.has_insn)
        {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            case OPCODE_ADD:
            {
                // condition check for anti, flow and output dependancies, if true skip cycle
                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                //  set flag to stop instruction being fetch in fetch stage {do condition check in fetch stage}
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }

                // destination register is invalid, making flag = 1 = invalid
                flags[cpu->decode.rd]++;

                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_SUB:
            {
                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_MUL:
            {

                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_DIV:
            {

                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_AND:
            {

                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_OR:
            {

                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_XOR:
            {

                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_ADDL:
            {
                
                if(flags[cpu->decode.rs1] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_SUBL:
            {
                // if(flags[cpu->decode.rd] == 1)
                // {
                //    temp = 1;
                // }
                if(flags[cpu->decode.rs1] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_LOAD:
            {

                if(flags[cpu->decode.rs1] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_LDR:
            {

                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0 /*|| flags[cpu->decode.rd] == 1*/){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }
                flags[cpu->decode.rd]++;
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_STORE:
            {

                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }

                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_STR:
            {

                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0 || flags[cpu->decode.rs3] > 0){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }

                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                cpu->decode.rs3_value = cpu->regs[cpu->decode.rs3];
                break;
            }

            case OPCODE_CMP:
            {
                if(flags[cpu->decode.rs1] > 0 || flags[cpu->decode.rs2] > 0){
                stall = 1;
                if (ENABLE_DEBUG_MESSAGES && command_simulate == 0){
                print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
                }
                return;
                }

                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_NOP:
            {
                break;
            }

            case OPCODE_BZ:
            {
                break;
            }

            case OPCODE_BNZ:
            {
                break;
            }

            case OPCODE_HALT:
            {
                break;
            }

            case OPCODE_MOVC:
            {
                flags[cpu->decode.rd]++;
                break;
            }

           
        }

        /* Copy data from decode latch to execute latch*/
        cpu->execute = cpu->decode;
        cpu->decode.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
            print_stage_content("Instruction at DECODE_RF_STAGE --->", &cpu->decode);
        }

    }
    else
    {
        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
           printf("Instruction at DECODE_RF_STAGE --->      EMPTY \n");
        }
        
    }   
    
}

/*
Execute Stage of APEX Pipeline
*/
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value + cpu->execute.rs2_value;
                //printf("register name = %d",cpu->execute.rs1);

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }

                break;
            }

            case OPCODE_ADDL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_SUB:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->execute.result_buffer  = cpu->execute.rs1_value - cpu->execute.imm;
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_MUL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value * cpu->execute.rs2_value;
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_DIV:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value / cpu->execute.rs2_value;
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_AND:
            {
                cpu->execute.result_buffer  = cpu->execute.rs1_value & cpu->execute.rs2_value;
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_OR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_XOR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value ^ cpu->execute.rs2_value;
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                break;
            }

            case OPCODE_LDR:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.rs2_value;
                break;
            }

            case OPCODE_STORE:
            {
                cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
                break;
            }

            case OPCODE_STR:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.rs2_value;
                break;
            }

            case OPCODE_BZ:
            {
                if (cpu->zero_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNZ:
            {
                if (cpu->zero_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->execute.result_buffer = cpu->execute.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_CMP: 
            {
                if (cpu->execute.rs1_value == cpu->execute.rs2_value)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                
                break;
            }

            case OPCODE_NOP: 
            {
                break;
            }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
            print_stage_content("Instruction at EX_STAGE        --->", &cpu->execute);
        }
    }
    else
    {
        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
           printf("Instruction at EX_STAGE        --->      EMPTY \n");
        }
        
    }
}

/*
Memory Stage of APEX Pipeline
*/
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
            case OPCODE_MOVC:
            {
                break;
            }

            case OPCODE_ADD:
            {
                /* No work for ADD */
                break;
            }

            case OPCODE_ADDL:
            {
                break;
            }

            case OPCODE_SUB:
            {
                break;
            }

            case OPCODE_SUBL:
            {
                break;
            }

            case OPCODE_MUL:
            {
                break;
            }

            case OPCODE_DIV:
            {
                break;
            }

            case OPCODE_AND:
            {
                break;
            }

            case OPCODE_OR:
            {
                break;
            }

            case OPCODE_XOR:
            {
                break;
            }

            case OPCODE_LOAD:
            {
                /* Read from data memory */
                cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
                break;
            }

            case OPCODE_LDR:
            {
                /* Read from data memory */
                cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
                break;
            }

            case OPCODE_STORE:
            {
                /* Write from data memory */
                cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;
                break;
            }

            case OPCODE_STR:
            {
                /* Write from data memory */
                cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs3_value;
                break;
            }

            case OPCODE_CMP:
            {
                break;
            }

            case OPCODE_NOP:
            {
                break;
            }

        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
            print_stage_content("Instruction at MEMORY_STAGE    --->", &cpu->memory);
        }
    }
    else
    {
        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
           printf("Instruction at MEMORY STAGE    --->      EMPTY \n");
        }
        
    }
}

/*
Writeback Stage of APEX Pipeline
*/
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                // after writing result into register register is valid
                flags[cpu->writeback.rd]--;
                // resetting stalling so we can start fetching new instructions
                stall = 0;
                break;
            }

            case OPCODE_ADDL:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                break;
            }

            case OPCODE_SUB:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                // if(temp == 0)
                // {
                   flags[cpu->writeback.rd]--;
                   stall = 0;
                //}
                
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                break;
            }

            case OPCODE_MUL:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                break;
            }

            case OPCODE_DIV:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                break;
            }

            case OPCODE_AND:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                break;
            }

            case OPCODE_OR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                break;
            }

            case OPCODE_XOR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                //printf("Value at register %d = %d \n", cpu->writeback.rd, cpu->writeback.result_buffer);
                break;
            }

            case OPCODE_LDR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                //printf("register name = %d and result = %d \n",cpu->writeback.rd, cpu->writeback.result_buffer);
                break;
            }

            case OPCODE_STORE:
            {
                //printf("result at memory address %d is = %d \n", cpu->writeback.memory_address, cpu->data_memory[cpu->writeback.memory_address]);
                break;
            }

            case OPCODE_STR:
            {
                //printf("result at memory address %d is = %d \n", cpu->writeback.memory_address, cpu->data_memory[cpu->writeback.memory_address]);
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                flags[cpu->writeback.rd]--;
                stall = 0;
                break;
            }
            
            case OPCODE_CMP:
            {
                break;
            }

            case OPCODE_NOP:
            {
                break;
            }
            

        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
            print_stage_content("Instruction at WRITEBACK_STAGE --->", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }
    else
    {
        if (ENABLE_DEBUG_MESSAGES && command_simulate == 0)
        {
           printf("Instruction at WRITEBACK_STAGE --->      EMPTY \n");
        }
        
    }

    /* Default */
    return 0;
}

/*
This function creates and initializes APEX cpu.
*/
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    cpu->clock = 1;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);

    
    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "rs3", "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].rs3, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
APEX CPU simulation loop
 */
void
APEX_cpu_run(APEX_CPU *cpu, const char *command, const char *step)
{
    char user_prompt_val;
    int val = atoi(step);
     
    
    if(strcmp(command, "simulate") == 0 || strcmp(command, "show_mem") == 0)
    {
        command_simulate = 1;
    }

    if(strcmp(command,"single_step") == 0)
    {
      cpu->single_step = ENABLE_SINGLE_STEP;
      while (TRUE)
     {
        if (ENABLE_DEBUG_MESSAGES)
        {
                printf("--------------------------------------------\n");
                printf("Clock Cycle #: %d\n", cpu->clock);
                printf("--------------------------------------------\n");
            
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
               printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
               break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
    printf("\n");
    print_reg_flag(cpu);
    printf("\n");
    print_mem(cpu);
    printf("\n");
    }
    else if(strcmp(command, "show_mem") == 0)
    {
     while (TRUE)
    {
        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            // if(strcmp(command, "display") == 0)
            // {
               printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
               break;
            //}
            
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        cpu->clock++;
    }
    printf("\n");
    print_reg_flag(cpu);
    printf("\n");
    print_mem(cpu);
    printf("\n");
    printf("value at %d memory location is %d. \n",val,cpu->data_memory[val]);
    }
    else // simulate and display
    {
        while (cpu->clock <= val)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            if(strcmp(command, "display") == 0)
            {
                printf("--------------------------------------------\n");
                printf("Clock Cycle #: %d\n", cpu->clock);
                printf("--------------------------------------------\n");
            }
            
        }

        if (APEX_writeback(cpu))
        {
               printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
               break;
            
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        cpu->clock++;
    }

    printf("\n");
    print_reg_flag(cpu);
    printf("\n");
    print_mem(cpu);
    printf("\n");
    
    cpu->single_step = ENABLE_SINGLE_STEP;

    if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            }
        }

           while (TRUE)
     {
        if (ENABLE_DEBUG_MESSAGES)
        {
                printf("--------------------------------------------\n");
                printf("Clock Cycle #: %d\n", cpu->clock);
                printf("--------------------------------------------\n");
            
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
               printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
               break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
    printf("\n");
    print_reg_flag(cpu);
    printf("\n");
    print_mem(cpu);
    printf("\n");

    }
    
    
    
}

/*
This function deallocates APEX CPU.
*/
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}