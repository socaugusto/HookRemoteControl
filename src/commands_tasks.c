#include "commands.h"
#include "database.h"
#include "spin3204_control.h"

CommandState_e executeCmdTaskHoming(CommandObject_t *cmdObject)
{
    switch (cmdObject->state)
    {
    case COMMAND_STATE_START:
        cmdObject->state = COMMAND_STATE_ACTION;

        break;
    case COMMAND_STATE_SETUP:

        break;
    case COMMAND_STATE_ACTION:
        mc_moveTo(database_convertTargetToValue(HOOK_TARGET_HOMING), database_getHomingSpeed(), MC_MODE_CONSTANT_SPEED);
        cmdObject->state = COMMAND_STATE_END;

        break;
    case COMMAND_STATE_TEARDOWN:

        break;
    case COMMAND_STATE_END:
        if (database_getState() == HOOK_STATE_CLOSED)
        {
            cmdObject->state = COMMAND_STATE_FINISH;
            mc_eack();
        }

        cmdObject->state = COMMAND_STATE_FINISH; // TEMPORARY

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