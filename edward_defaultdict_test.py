from collections import defaultdict
from threading import Barrier, Condition, Thread


def test():
    cv = Condition()

    key = 'my-key'
    setting_ready_flag = False

    def default_factory():
        with cv:
            while not setting_ready_flag:
                print("Waiting for value to be set...")
                cv.wait()
            print("missing value is set, returning it.")
            return "default"

    mydict = defaultdict(default_factory)

    def writer():
        with cv:
            nonlocal setting_ready_flag
            mydict[key] = "setted value"
            setting_ready_flag = True
            print("Value set, notifying all waiting threads.")
            cv.notify()
        
    default_fact_thread = Thread(target=lambda: mydict[key])
    writer_thread = Thread(target=writer)
    default_fact_thread.start()
    writer_thread.start()

    default_fact_thread.join()
    writer_thread.join()

    print(mydict.items())  # Should print "setted value" but actually "default"
    
test()