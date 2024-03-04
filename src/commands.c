#include "commands.h"
#include "database.h"
#include <zephyr/logging/log.h>
#include <memory.h>

#define LOG_MODULE_NAME commands
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define MAX_NUMBER_OF_COMMANDS 8
static CommandInput_t cmdBuffer[MAX_NUMBER_OF_COMMANDS];
static int32_t cmdIdxStore = 0; // Head
static int32_t cmdIdxUse = 0;   // Tail
static int32_t cmdCount = 0;
static CommandInput_t *cmd = NULL;
static CommandObject_t cmdObject;

static CommandInput_t *process(CommandInput_t *cmd, CommandState_e state);

void command_addToBuffer(CommandInput_t *cmd)
{
    if (cmdCount < MAX_NUMBER_OF_COMMANDS)
    {
        memcpy(cmdBuffer + cmdIdxStore, cmd, sizeof(CommandInput_t));
        cmdIdxStore = (cmdIdxStore + 1) % MAX_NUMBER_OF_COMMANDS;
        ++cmdCount;
    }
    else
    {
        /// TODO(Silvio): Handle error
        LOG_ERR("[ERROR] Exceeded command buffer capacity");
    }
}

void command_run(void)
{
    if (cmdCount != 0 && cmd == NULL)
    {
        cmd = &cmdBuffer[cmdIdxUse];
        cmdIdxUse = (cmdIdxUse + 1) % MAX_NUMBER_OF_COMMANDS;
        --cmdCount;

        switch (cmd->operation)
        {
        case COMMAND_NONE:
            cmd = NULL;

            break;
        case COMMAND_HOMING:
            cmdObject.operation = cmd->operation;
            cmdObject.task = executeCmdTaskHoming;

            break;
        case COMMAND_EACK:
            cmdObject.operation = cmd->operation;
            cmdObject.task = executeCmdTaskEack;

            break;
        case COMMAND_SYSTEM_RESET:
            cmdObject.operation = cmd->operation;
            cmdObject.task = executeCmdTaskReboot;

            break;
        case COMMAND_STOP:
            cmdObject.operation = cmd->operation;
            cmdObject.task = executeCmdTaskStop;

            break;
        case COMMAND_HOOK_CLOSE:
            cmdObject.operation = cmd->operation;
            cmdObject.task = executeCmdClose;

            break;
        case COMMAND_HOOK_MID:
            cmd = NULL;

            break;
        case COMMAND_HOOK_OPEN:
            cmd = NULL;

            break;
        default:

            break;
        }
    }

    if (cmd) // exists, then execute its task and process result
    {
        cmd = process(cmd, cmdObject.task(&cmdObject));
    }
}

static CommandInput_t *process(CommandInput_t *cmd, CommandState_e state)
{
    return (COMMAND_STATE_FINISH == state ? NULL : cmd);
}