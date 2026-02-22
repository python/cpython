"""Batch 21: Document packages (part 2) + misc"""
import sys
results = []

# pylatex
try:
    from pylatex import Document
    doc = Document("test")
    doc.append("Hello Nanvix")
    results.append(("pylatex", "PASS"))
except Exception as e: results.append(("pylatex", f"FAIL: {e}"))

# chess
try:
    import chess
    board = chess.Board()
    assert board.is_valid()
    board.push_san("e4")
    assert len(list(board.legal_moves)) > 0
    results.append(("chess", "PASS"))
except Exception as e: results.append(("chess", f"FAIL: {e}"))

# shellingham
try:
    import shellingham
    results.append(("shellingham", "PASS"))
except Exception as e: results.append(("shellingham", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
