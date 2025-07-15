        case 0: {
            break;
        }

        case 1: {
            if (CURRENT_OPARG()) {
                JUMP_TO_JUMP_TARGET();
            }
            break;
        }

        case 2: {
            if (CURRENT_OPARG()) {
                JUMP_TO_ERROR();
            }
            break;
        }

        case 3: {
            GOTO_TIER_ONE((void *)CURRENT_OPERAND0() + CURRENT_TARGET());
            break;
        }

        case 4: {
            GOTO_TIER_TWO((void *)CURRENT_OPERAND1());
            break;
        }