"""Batch 20: Graphs + fonts (heavy imports)"""
import sys
results = []

# networkx
try:
    import networkx as nx
    G = nx.Graph()
    G.add_edges_from([(1,2),(2,3),(3,4)])
    path = nx.shortest_path(G, 1, 4)
    assert path == [1, 2, 3, 4]
    results.append(("networkx", "PASS"))
except Exception as e: results.append(("networkx", f"FAIL: {e}"))

# fonttools
try:
    from fontTools.ttLib import TTFont
    results.append(("fonttools", "PASS"))
except Exception as e: results.append(("fonttools", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
