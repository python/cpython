# IMPORT
from typing import Callable, Any
import hmac

class HKDF:
    """
    HMAC-based Key Derivation Function (HKDF) implementation following RFC 5869.
    """
    def __init__(self, ikm: bytes, salt: bytes | None, algorithm: Callable[..., Any]) -> None:
        """
        Initialize HKDF with:
        - ikm: Input Keying Material (bytes)
        - salt: Optional salt (bytes). If None, defaults to zeros of hash length.
        - algorithm: Hash function constructor (e.g., hashlib.sha256)
        """
        self.hashalgorithm = algorithm
        self.hashlen: int = self.hashalgorithm().digest_size
        self.prk: bytes = self.extract(ikm=ikm, salt=salt)
        #
        return
    #
    def extract(self, ikm: bytes, salt: bytes | None) -> bytes:
        """
        HKDF-Extract(salt, IKM) -> PRK
        - Computes the pseudorandom key (PRK) using HMAC.
        - If salt is None, RFC 5869 specifies using a string of zeros of length HashLen.
        """
        salt = b"\x00" * self.hashlen if salt is None else salt
        #
        return hmac.new(salt, ikm, self.hashalgorithm).digest()
    #
    def expand(self, length: int = 32, info: bytes | None = None) -> bytes:
        """
        HKDF-Expand(PRK, info, L) -> OKM
        - Generates output key material (OKM) of desired length.
        - Parameters:
            - length: Number of bytes of OKM to produce
            - info: Optional context/application-specific information
        """
        if length > 255 * self.hashlen:
            raise ValueError(f"Cannot expand to more than {255 * self.hashlen} bytes")
        #
        info = b"" if info is None else info
        pblock: bytes = b""
        okm: bytes = b""
        #
        blocksneeded = (length + self.hashlen - 1) // self.hashlen
        for counter in range(1, blocksneeded + 1):
            pblock = hmac.new(self.prk, pblock + info + bytes([counter]), self.hashalgorithm).digest()
            okm += pblock
        #
        return okm[:length]
    #
    def derive(self, length: int = 32, info: bytes | None = None) -> bytes:
        """
        Convenience wrapper for HKDF-Expand.
        - Calls expand() with given length and info.
        - Returns derived key material (OKM).
        """
        return self.expand(length=length, info=info)
