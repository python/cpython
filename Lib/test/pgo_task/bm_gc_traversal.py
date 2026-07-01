import gc

N_LEVELS = 1000


def create_recursive_containers(n_levels):

    current_list = []
    for n in range(n_levels):
        new_list = [None] * n
        for index in range(n):
            new_list[index] = current_list
        current_list = new_list

    return current_list


def benchamark_collection(loops, n_levels):
    all_cycles = create_recursive_containers(n_levels)
    for _ in range(loops):
        gc.collect()
        # Main loop to measure
        collected = gc.collect()

        assert collected is None or collected == 0



def run_pgo():
    benchamark_collection(10, N_LEVELS)
