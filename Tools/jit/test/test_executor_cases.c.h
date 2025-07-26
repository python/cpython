        case 0: {
            // Zero-length jumps should be removed:
            break;
        }

        case 1: {
            // -Os duplicates less code than -O3:
            PyAPI_DATA(bool) sausage;
            PyAPI_DATA(bool) spammed;
            PyAPI_FUNC(void) order_eggs_and_bacon(void);
            PyAPI_FUNC(void) order_eggs_sausage_and_bacon(void);
            if (!sausage) {
                order_eggs_and_bacon();
            }
            else {
                order_eggs_sausage_and_bacon();
            }
            spammed = false;
            break;
        }

        case 2: {
            // The assembly optimizer inverts hot branches:
            PyAPI_DATA(bool) spam;
            if (spam) {
                JUMP_TO_ERROR();
            }
            break;
        }
