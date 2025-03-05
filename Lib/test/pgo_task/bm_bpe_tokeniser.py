"""
Benchmark a BPE tokeniser.

Author: Shantanu Jain

Based on code from tiktoken.
https://github.com/openai/tiktoken
"""
from __future__ import annotations

import collections
import re
from pathlib import Path


class SimpleBytePairEncoding:
    def __init__(self, *, pat_str: str, mergeable_ranks: dict[bytes, int]) -> None:
        self.pat_str = pat_str
        self.mergeable_ranks = mergeable_ranks

        self._decoder = {token: token_bytes for token_bytes, token in mergeable_ranks.items()}
        self._pat = re.compile(pat_str)

    def encode(self, text: str) -> list[int]:
        # Use the regex to split the text into (approximately) words
        words = self._pat.findall(text)
        tokens = []
        for word in words:
            # Turn each word into tokens, using the byte pair encoding algorithm
            word_bytes = word.encode("utf-8")
            word_tokens = bpe_encode(self.mergeable_ranks, word_bytes)
            tokens.extend(word_tokens)
        return tokens

    def decode_bytes(self, tokens: list[int]) -> bytes:
        return b"".join(self._decoder[token] for token in tokens)

    def decode(self, tokens: list[int]) -> str:
        return self.decode_bytes(tokens).decode("utf-8", errors="replace")

    @staticmethod
    def train(training_data: str, vocab_size: int, pat_str: str):
        mergeable_ranks = bpe_train(data=training_data, vocab_size=vocab_size, pat_str=pat_str)
        return SimpleBytePairEncoding(pat_str=pat_str, mergeable_ranks=mergeable_ranks)


def bpe_encode(mergeable_ranks: dict[bytes, int], input: bytes) -> list[int]:
    # A simple, uncached, quadratic BPE
    parts = [bytes([b]) for b in input]
    while True:
        # Iterate over all pairs and find the pair we want to merge the most
        min_idx = None
        min_rank = None
        for i, pair in enumerate(zip(parts[:-1], parts[1:])):
            rank = mergeable_ranks.get(pair[0] + pair[1])
            if rank is not None and (min_rank is None or rank < min_rank):
                min_idx = i
                min_rank = rank

        # If there were no pairs we could merge, we're done!
        if min_rank is None:
            break
        assert min_idx is not None

        # Otherwise, merge that pair and leave the rest unchanged. Then repeat.
        parts = parts[:min_idx] + [parts[min_idx] + parts[min_idx + 1]] + parts[min_idx + 2 :]

    tokens = [mergeable_ranks[part] for part in parts]
    return tokens


def bpe_train(data: str, vocab_size: int, pat_str: str) -> dict[bytes, int]:
    # First, add tokens for each individual byte value
    if vocab_size < 2**8:
        raise ValueError("vocab_size must be at least 256, so we can encode all bytes")
    ranks = {}
    for i in range(2**8):
        ranks[bytes([i])] = i

    # Splinter up our data into lists of bytes
    # data = "Hello world"
    # words = [
    #     [b'H', b'e', b'l', b'l', b'o'],
    #     [b' ', b'w', b'o', b'r', b'l', b'd']
    # ]
    words: list[list[bytes]] = [
        [bytes([b]) for b in word.encode("utf-8")] for word in re.findall(pat_str, data)
    ]

    # Now, use our data to figure out which merges we should make
    while len(ranks) < vocab_size:
        # Find the most common pair. This will become our next token
        stats = collections.Counter()
        for piece in words:
            for pair in zip(piece[:-1], piece[1:]):
                stats[pair] += 1

        most_common_pair = max(stats, key=lambda x: stats[x])
        token_bytes = most_common_pair[0] + most_common_pair[1]
        token = len(ranks)
        # Add the new token!
        ranks[token_bytes] = token

        # Now merge that most common pair in all the words. That is, update our training data
        # to reflect our decision to make that pair into a new token.
        new_words = []
        for word in words:
            new_word = []
            i = 0
            while i < len(word) - 1:
                if (word[i], word[i + 1]) == most_common_pair:
                    # We found our pair! Merge it
                    new_word.append(token_bytes)
                    i += 2
                else:
                    new_word.append(word[i])
                    i += 1
            if i == len(word) - 1:
                new_word.append(word[i])
            new_words.append(new_word)
        words = new_words

    return ranks


def train(data: str):
    pattern = (
        r"""'s|'t|'re|'ve|'m|'ll|'d| ?[a-zA-Z]+| ?\d+| ?[^\sa-zA-Z\d]+|\s+(?!\S)|\s+"""
    )
    enc = SimpleBytePairEncoding.train(data, vocab_size=1024, pat_str=pattern)

    tokens = enc.encode("hello world")
    assert enc.decode(tokens) == "hello world"

    enc.encode(data)


def bench_bpe_tokeniser(loops: int) -> float:
    DATA = Path(__file__).parent / "data" / "frankenstein_intro.txt"
    with open(DATA, "r", encoding="utf8") as f:
        data = f.read()

    range_it = range(loops)

    for _ in range_it:
        train(data)


def run_pgo():
    bench_bpe_tokeniser(1)
