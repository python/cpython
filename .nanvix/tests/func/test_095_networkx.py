"""Test: networkx"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import networkx as nx
    G = nx.Graph()
    G.add_edges_from([(1, 2), (2, 3), (3, 1)])
    assert len(G.nodes) == 3
    assert nx.is_connected(G)
    print("networkx: PASS")
except Exception as e:
    print(f"networkx: FAIL: {e}")
    sys.exit(1)
