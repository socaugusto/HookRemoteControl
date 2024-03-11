#include "commands.h"
#include "database.h"
#include "spin3204_control.h"
#include "remote.h"

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME cmd_task
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

CommandState_e executeCmdTaskHoming(CommandObject_t *cmdObject)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        if (database_isAtEndStroke())
        {
            mc_eack();
            cmdObject->state = COMMAND_STATE_ACTION;
        }
        else
        {
            mc_moveTo(database_convertTargetToValue(HOOK_TARGET_HOMING), database_getHomingSpeed(), MC_MODE_CONSTANT_SPEED);
            LOG_INF("Sent move to home");
            cmdObject->state = COMMAND_STATE_SETUP;
        }

        break;
    case COMMAND_STATE_SETUP:
        if (database_isAtEndStroke())
        {
            LOG_INF("End of stroke reached");
            mc_setPositionHome();
            cmdObject->state = COMMAND_STATE_END;
        }
        else if (database_getError() == ERROR_OVERLOAD)
        {
            LOG_INF("Overload error detected");
            mc_eack();
            cmdObject->state = COMMAND_STATE_ACTION;
        }

        break;
    case COMMAND_STATE_ACTION:
        if (database_getError() == ERROR_NONE)
        {
            LOG_INF("Error cleared");
            mc_setPositionHome();
            cmdObject->state = COMMAND_STATE_END;
        }

        break;
    case COMMAND_STATE_TEARDOWN:
    case COMMAND_STATE_END:
        remote_updateHookState(HOOK_STATE_CLOSED);
        LOG_INF("Command finished homing successfull");
        cmdObject->state = COMMAND_STATE_FINISH;

        break;
    case COMMAND_STATE_FINISH:
    default:

        break;
    }

    return cmdObject->state;
}

CommandState_e executeCmdTaskEack(CommandObject_t *cmdObject)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        cmdObject->state = COMMAND_STATE_ACTION;

        break;
    case COMMAND_STATE_SETUP:

        break;
    case COMMAND_STATE_ACTION:
        mc_eack();
        cmdObject->state = COMMAND_STATE_FINISH;

        break;
    case COMMAND_STATE_TEARDOWN:

        break;
    case COMMAND_STATE_END:

        break;
    case COMMAND_STATE_FINISH:
    default:

        break;
    }

    return cmdObject->state;
}

CommandState_e executeCmdTaskStop(CommandObject_t *cmdObject)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        cmdObject->state = COMMAND_STATE_ACTION;

        break;
    case COMMAND_STATE_SETUP:

        break;
    case COMMAND_STATE_ACTION:
        mc_stop();
        cmdObject->state = COMMAND_STATE_FINISH;

        break;
    case COMMAND_STATE_TEARDOWN:

        break;
    case COMMAND_STATE_END:

        break;
    case COMMAND_STATE_FINISH:
    default:

        break;
    }

    return cmdObject->state;
}

CommandState_e executeCmdTaskReboot(CommandObject_t *cmdObject)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        cmdObject->state = COMMAND_STATE_ACTION;

        break;
    case COMMAND_STATE_SETUP:

        break;
    case COMMAND_STATE_ACTION:
        mc_reboot();
        cmdObject->state = COMMAND_STATE_FINISH;

        break;
    case COMMAND_STATE_TEARDOWN:

        break;
    case COMMAND_STATE_END:

        break;
    case COMMAND_STATE_FINISH:
    default:

        break;
    }

    return cmdObject->state;
}

CommandState_e executeCmdClose(CommandObject_t *cmdObject)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        cmdObject->state = COMMAND_STATE_ACTION;

        break;
    case COMMAND_STATE_SETUP:

        break;
    case COMMAND_STATE_ACTION:
        mc_moveTo(database_convertTargetToValue(HOOK_TARGET_CLOSED), database_getClosingSpeed(), MC_MODE_CONSTANT_SPEED);
        cmdObject->state = COMMAND_STATE_END;

        break;
    case COMMAND_STATE_TEARDOWN:

        break;
    case COMMAND_STATE_END:
        if (database_getState() == HOOK_STATE_CLOSED)
        {
            cmdObject->state = COMMAND_STATE_FINISH;
        }

        cmdObject->state = COMMAND_STATE_FINISH; // TEMPORARY

        break;
    case COMMAND_STATE_FINISH:
    default:

        break;
    }

    return cmdObject->state;
}