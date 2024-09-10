#include "commands.h"
#include "database.h"
#include "spin3204_control.h"
#include "remote.h"

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME cmd_task
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static int32_t timeout = 0;

CommandState_e executeCmdTaskHoming(CommandObject_t *cmdObject)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        if (database_isAtEndStroke())
        {
            mc_eack();
            cmdObject->state = COMMAND_STATE_TEARDOWN;
        }
        else
        {
            mc_setPositionUninitialized();
            LOG_INF("Reset position");
            cmdObject->state = COMMAND_STATE_SETUP;
        }

        break;
    case COMMAND_STATE_SETUP:
        mc_moveTo(database_convertTargetToValue(HOOK_TARGET_HOMING), database_getHomingSpeed(), database_getNextSeqNo());
        LOG_INF("Sent move to home");
        cmdObject->state = COMMAND_STATE_ACTION;

        break;
    case COMMAND_STATE_ACTION:
        if (database_isAtEndStroke() && database_isPositionEncoderHome())
        {
            LOG_INF("End of stroke reached");
            mc_setPositionHome();
            cmdObject->state = COMMAND_STATE_END;
        }
        else if (database_getError() == ERROR_OVERLOAD)
        {
            LOG_INF("Overload error detected");
            mc_eack();
            database_eackError();
            timeout = cmdObject->timer + 20;
            cmdObject->state = COMMAND_STATE_TEARDOWN;
        }

        break;
    case COMMAND_STATE_TEARDOWN:
        if(timeout < cmdObject->timer)
        {
            if (database_getError() == ERROR_NONE)
            {
                LOG_INF("Error cleared");
                mc_setPositionHome();
                cmdObject->state = COMMAND_STATE_END;
            }
            else
            {
                cmdObject->state = COMMAND_STATE_ACTION;
            }
        }

        break;
    case COMMAND_STATE_END:
        remote_updateHookState(HOOK_STATE_CLOSED);
        database_resetPosition();

        mc_setIgnoreSensorParameter(0);

        cmdObject->state = COMMAND_STATE_FINISH;
        LOG_INF("Command finished homing successfull");


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
        database_eackError();
        LOG_INF("Clearing error...");
        cmdObject->state = COMMAND_STATE_SETUP;

        break;
    case COMMAND_STATE_SETUP:
        if (cmdObject->timer > 40)
        {
            if (database_getError() == ERROR_NONE)
            {
                cmdObject->state = COMMAND_STATE_END;
                LOG_INF("Error cleared in remote");
            }
            else
            {
                mc_eack();
                database_eackError();
                cmdObject->state = COMMAND_STATE_ACTION;
                LOG_INF("Sending error clearing to hook...");
            }
        }

        break;
    case COMMAND_STATE_ACTION:
        if (cmdObject->timer > 80)
        {
            if (database_getError() == ERROR_NONE)
            {
                cmdObject->state = COMMAND_STATE_END;
                LOG_INF("Error cleared in hook");
            }
            else
            {
                database_eackError();
            }
        }

        break;
    case COMMAND_STATE_TEARDOWN:

        break;
    case COMMAND_STATE_END:
        remote_updateHookState(HOOK_STATE_UNINITIALIZED);
        cmdObject->state = COMMAND_STATE_FINISH;

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
        timeout = cmdObject->timer + 10;
        LOG_INF("Sent stop request...");
        cmdObject->state = COMMAND_STATE_TEARDOWN;

        break;
    case COMMAND_STATE_TEARDOWN:
        if (database_isStopped())
        {
            cmdObject->state = COMMAND_STATE_FINISH;
        }
        else if (timeout < cmdObject->timer)
        {
            cmdObject->state = COMMAND_STATE_ACTION;
        }

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
        LOG_INF("Reboot request...");
        cmdObject->state = COMMAND_STATE_END;

        break;
    case COMMAND_STATE_TEARDOWN:

        break;
    case COMMAND_STATE_END:
        if (database_getState() == HOOK_STATE_UNINITIALIZED)
        {
            cmdObject->state = COMMAND_STATE_FINISH;
            LOG_INF("Reboot confirmed!");
        }

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
        mc_setHardwareCurrentLimiter(true);
        cmdObject->state = COMMAND_STATE_SETUP;

        break;
    case COMMAND_STATE_SETUP:
        if (cmdObject->timer > 20)
        {
            cmdObject->state = COMMAND_STATE_ACTION;
        }

        break;
    case COMMAND_STATE_ACTION:
        mc_moveTo(database_convertTargetToValue(HOOK_TARGET_CLOSED), database_getClosingSpeed(), database_getNextSeqNo());
        cmdObject->state = COMMAND_STATE_TEARDOWN;

        break;
    case COMMAND_STATE_TEARDOWN:
        if (database_getState() == HOOK_STATE_CLOSED)
        {
            mc_setPositionHome();
            database_resetPosition();
            cmdObject->state = COMMAND_STATE_END;
            timeout = cmdObject->timer + 30;
        }

        break;
    case COMMAND_STATE_END:
        if (database_getState() == HOOK_STATE_CLOSED && timeout < cmdObject->timer)
        {
            database_printHookPosition();
            cmdObject->state = COMMAND_STATE_FINISH;
        }

        break;
    case COMMAND_STATE_FINISH:
    default:

        break;
    }

    return cmdObject->state;
}

static CommandState_e executeCmdMid(CommandObject_t *cmdObject, int16_t speed)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        mc_setCurrentLimitParameter(database_getCurrentAt(CURRENT_LIMIT_OPERATION));
        cmdObject->state = COMMAND_STATE_SETUP;

        break;
    case COMMAND_STATE_SETUP:
        mc_setHardwareCurrentLimiter(false);
        cmdObject->state = COMMAND_STATE_ACTION;

        break;
    case COMMAND_STATE_ACTION:
        if (cmdObject->timer > 20)
        {
            mc_moveTo(database_convertTargetToValue(HOOK_TARGET_MID), speed, database_getNextSeqNo());
            LOG_INF("Send command mid...");
            cmdObject->state = COMMAND_STATE_END;
            timeout = cmdObject->timer + 100;
        }

        break;
    case COMMAND_STATE_TEARDOWN:

        break;
    case COMMAND_STATE_END:
        if (database_getState() == HOOK_STATE_MID)
        {
            database_printHookPosition();
            cmdObject->state = COMMAND_STATE_FINISH;
        }
        else if (timeout < cmdObject->timer)
        {
            timeout = cmdObject->timer + 100;
            if (database_isStopped())
            {
                database_setError(ERROR_MOTOR_JAMMED);
            }
        }

        break;
    case COMMAND_STATE_FINISH:
    default:

        break;
    }

    return cmdObject->state;
}

CommandState_e executeCmdMidClose(CommandObject_t *cmdObject)
{
    return executeCmdMid(cmdObject, database_getClosingSpeed());
}
CommandState_e executeCmdMidOpen(CommandObject_t *cmdObject)
{
    return executeCmdMid(cmdObject, database_getOpeningSpeed());
}

CommandState_e executeCmdOpen(CommandObject_t *cmdObject)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        mc_setCurrentLimitParameter(database_getCurrentAt(CURRENT_LIMIT_OPERATION));
        cmdObject->state = COMMAND_STATE_SETUP;

        break;
    case COMMAND_STATE_SETUP:
        mc_setHardwareCurrentLimiter(false);
        cmdObject->state = COMMAND_STATE_ACTION;

        break;
    case COMMAND_STATE_ACTION:
        if (cmdObject->timer > 20)
        {
            mc_moveTo(database_convertTargetToValue(HOOK_TARGET_OPEN), database_getOpeningSpeed(), database_getNextSeqNo());
            cmdObject->state = COMMAND_STATE_END;
            timeout = cmdObject->timer + 100;
        }

        break;
    case COMMAND_STATE_TEARDOWN:

        break;
    case COMMAND_STATE_END:
        if (database_getState() == HOOK_STATE_OPEN)
        {
            database_printHookPosition();
            cmdObject->state = COMMAND_STATE_FINISH;
        }
        else if (timeout < cmdObject->timer)
        {
            timeout = cmdObject->timer + 100;
            if (database_isStopped())
            {
                database_setError(ERROR_MOTOR_JAMMED);
            }
        }

        break;
    case COMMAND_STATE_FINISH:
    default:

        break;
    }

    return cmdObject->state;
}

CommandState_e executeEnableRecovery(CommandObject_t *cmdObject)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        mc_setIgnoreSensorParameter(1);
        LOG_INF("Send ignore end of stroke...");
        cmdObject->state = COMMAND_STATE_SETUP;

        break;
    case COMMAND_STATE_SETUP:
        if (cmdObject->timer > 20)
        {
            mc_readParameter(PARAMETER_IGNORE_SENSOR);
            LOG_INF("Request parameter %d", PARAMETER_IGNORE_SENSOR);
            cmdObject->state = COMMAND_STATE_ACTION;
        }

        break;
    case COMMAND_STATE_ACTION:
        if (cmdObject->timer > 40)
        {
            mc_setHardwareCurrentLimiter(true);
            LOG_INF("Set limit current hardware %d", PARAMETER_CURRENT_LIMIT_TYPE);
            cmdObject->state = COMMAND_STATE_TEARDOWN;
        }

        break;
    case COMMAND_STATE_TEARDOWN:
        if (cmdObject->timer > 60)
        {
            mc_readParameter(PARAMETER_CURRENT_LIMIT_TYPE);
            LOG_INF("Request parameter %d", PARAMETER_CURRENT_LIMIT_TYPE);
            cmdObject->state = COMMAND_STATE_FINISH;
        }

        break;
    case COMMAND_STATE_END:

        break;
    case COMMAND_STATE_FINISH:
    default:

        break;
    }

    return cmdObject->state;
}