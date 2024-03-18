#include "commands.h"
#include "database.h"
#include <zephyr/logging/log.h>
#include <memory.h>
#include "remote.h"

#define LOG_MODULE_NAME commands
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define TIMEOUT_COMMAND 800
#define MAX_NUMBER_OF_COMMANDS 8
static CommandInput_t cmdBuffer[MAX_NUMBER_OF_COMMANDS];
static int32_t cmdIdxStore = 0; // Head
static int32_t cmdIdxUse = 0;   // Tail
static int32_t cmdCount = 0;
static CommandInput_t *cmd = NULL;
static CommandObject_t cmdObject;

static CommandInput_t *process(CommandInput_t *cmd, CommandObject_t *cmdObject);
static CommandInput_t *requestStop(void);

uint8_t command_isInExecution(void)
{
    return (cmd != NULL);
}

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

void command_flush(void)
{
    cmd = NULL;
    cmdCount = 0;
    cmdIdxUse = 0;
    cmdIdxStore = 0;
}

void command_run(void)
{
    if (cmdCount != 0 && cmd == NULL)
    {
        cmdObject.state = COMMAND_STATE_START;
        cmdObject.timer = 0;
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
        case COMMAND_HOOK_MID_CLOSE:
            cmdObject.operation = cmd->operation;
            cmdObject.task = executeCmdMidClose;

            break;
        case COMMAND_HOOK_OPEN:
            cmdObject.operation = cmd->operation;
            cmdObject.task = executeCmdOpen;

            break;
        case COMMAND_HOOK_MID_OPEN:
            cmdObject.operation = cmd->operation;
            cmdObject.task = executeCmdMidOpen;

        default:

            break;
        }
    }

    if (cmd) // exists, then execute its task and process result
    {
        cmd = process(cmd, &cmdObject);
    }
}

static CommandInput_t *process(CommandInput_t *cmd, CommandObject_t *cmdObject)
{
    ++cmdObject->timer;
    CommandInput_t *result = cmd;
    CommandState_e state = cmdObject->task(cmdObject);

    if (cmdObject->timer > TIMEOUT_COMMAND)
    {
        database_setError(ERROR_COMMAND_TIMEOUT);
        result = requestStop();
        LOG_WRN("[ERROR] Timeout, request stop...");
    }
    else if (database_getError() && (cmdObject->operation != COMMAND_STOP) && (cmdObject->operation != COMMAND_EACK))
    {
        LOG_WRN("[ERROR] Error, request stop...");
        result = requestStop();
    }

    if (COMMAND_STATE_FINISH == state)
    {
        result = NULL;
        LOG_INF("Command terminated!");
    }

    return result;
}

static CommandInput_t *requestStop(void)
{
    remote_updateHookState(HOOK_STATE_ERROR);
    CommandInput_t cmd = {.operation = COMMAND_STOP};
    command_addToBuffer(&cmd);
    return NULL;
}