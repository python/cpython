try:
    import idle
except SystemExit:
    raise
except:
    import traceback
    traceback.print_exc()
    raw_input("Hit return to exit...")
