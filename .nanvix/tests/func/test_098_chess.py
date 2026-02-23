"""Test: chess"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import chess
    board = chess.Board()
    assert board.is_valid()
    assert len(list(board.legal_moves)) == 20
    print("chess: PASS")
except Exception as e:
    print(f"chess: FAIL: {e}")
    sys.exit(1)
