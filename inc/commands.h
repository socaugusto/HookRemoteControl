#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include <stdint.h>

typedef enum Command_e_
{
    COMMAND_NONE,
    COMMAND_HOMING,
    COMMAND_EACK,
    COMMAND_SYSTEM_RESET,
    COMMAND_STOP,
    COMMAND_HOOK_CLOSE = 128,
    COMMAND_HOOK_MID = 136,
    COMMAND_HOOK_OPEN = 142
} Command_e;

typedef enum CommandState_e_
{
    COMMAND_STATE_START,
    COMMAND_STATE_SETUP,
    COMMAND_STATE_ACTION,
    COMMAND_STATE_TEARDOWN,
    COMMAND_STATE_END,
    COMMAND_STATE_FINISH,
} CommandState_e;
typedef struct CommandInput_t_
{
    Command_e operation;
    int32_t parameter1;
    int32_t parameter2;
    int32_t parameter3;
} CommandInput_t;

typedef struct CommandObject_t_
{
    Command_e operation;
    CommandState_e state;
    CommandState_e (*task)(struct CommandObject_t_ *);
} CommandObject_t;

void command_addToBuffer(CommandInput_t *cmd);
uint8_t command_isInExecution(void);
void command_run(void);

CommandState_e executeCmdTaskHoming(CommandObject_t *);
CommandState_e executeCmdTaskEack(CommandObject_t *);
CommandState_e executeCmdTaskStop(CommandObject_t *);
CommandState_e executeCmdTaskReboot(CommandObject_t *);
CommandState_e executeCmdClose(CommandObject_t *);

#endif