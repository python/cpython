fractions
=========

This document describes the working and implementation details of the
[`fractions`](https://docs.python.org/3/library/fractions.html) module,
providing some proofs for algorithms used.


# Proof of correctness of the `limit_denominator` algorithm

## The problem being solved

The `limit_denominator` algorithm takes a fraction $m/n$ and a positive integer $\ell$, and aims to return a "best" approximation to $m/n$ from amongst all fractions $r/s$ with denominator bounded by $\ell$. To make "best" precise, let's establish some conventions and terminology:

- All fractions are assumed to be in lowest terms, with positive denominator.
- A **candidate approximation** to $m / n$ with denominator bounded by $\ell$ (or just **candidate** for short) is a fraction $r/s$ with $s \le \ell$.
- Given two candidates $r/s$ and $t/u$, say that $r/s$ is **better** than $t/u$ if _either_
  - $r/s$ is closer to $m/n$ than $t/u$ is, _or_
  - $r/s$ and $t/u$ are equidistant from $m/n$, and $s \le u$.
- A candidate $r/s$ is a **best approximation** if it's better than all (other) candidates.

The goal of this exposition is to convince the reader that Python's implementation of `limit_denominator` always returns a best approximation for the given inputs.

Some notes:

- There's a tension with the English language: our "better" relation is reflexive (a candidate $r/s$ is better than itself), while the word "better" suggests an irreflexive relation. But other more accurate wordings ("no worse than", "better or equally good", "at least as good") are more cumbersome, so we'll live with the tension.
- We'll show below that outside one corner case, there's a _unique_ best approximation for a given set of arguments. That corner case is the case where $m/n$ is exactly halfway between two consecutive integers and $\ell = 1$; we'll call this the ambiguous case, and we'll return to it below.
- For an example of the tie-break condition for "better", consider the case where $m/n = 99/140 = 0.707142...$ and $\ell = 10$. Then $7/10 = 0.7$ and $5/7 = 0.714285...$ are candidate approximations, they're equidistant from the target fraction $99/140$, and they're each closer than any other candidate - any fraction that's closer must be strictly between $7/10$ and $5/7$, and all fractions in the open interval $(7/10, 5/7)$ have denominator larger than $10$. Since $5/7$ has smaller denominator than $7/10$, our definition says that $5/7$ is better than $7/10$ (but not vice versa). So $5/7$ is the unique best approximation.

## Simplifying the code

Here's the standard library implementation of `Fraction.limit_denominator`, shorn of
comments and docstring details, but otherwise unaltered.

```python
class Fraction(numbers.Rational):

    # other methods elided ...

    def limit_denominator(self, max_denominator=1000000):
        """Closest Fraction to self with denominator at most max_denominator."""
        if max_denominator < 1:
            raise ValueError("max_denominator should be at least 1")
        if self._denominator <= max_denominator:
            return Fraction(self)

        p0, q0, p1, q1 = 0, 1, 1, 0
        n, d = self._numerator, self._denominator
        while True:
            a = n//d
            q2 = q0+a*q1
            if q2 > max_denominator:
                break
            p0, q0, p1, q1 = p1, q1, p0+a*p1, q2
            n, d = d, n-a*d
        k = (max_denominator-q0)//q1

        if 2*d*(q0+k*q1) <= self._denominator:
            return Fraction._from_coprime_ints(p1, q1)
        else:
            return Fraction._from_coprime_ints(p0+k*p1, q0+k*q1)
```

Instead of proving correctness of the above code directly, we'll prove correctness
of an equivalent simplified version. Here's that version:

```python
def limit_denominator(m: int, n: int, l: int) -> tuple[int, int]:
    """
    Given a fraction m/n and a positive integer l, return integers r and s such
    that r/s is the closest fraction to m/n with denominator bounded by l.

    m/n need not be in lowest terms, but n must be positive.

    On return, 0 < s <= l and gcd(r, s) = 1.
    """
    if l < 1:
        raise ValueError("max_denominator should be at least 1")

    a, b, p, q, r, s, v = n, m % n, 1, 0, m // n, 1, 1
    while 0 < b and q + a // b * s <= l:
        a, b, p, q, r, s, v = b, a % b, r, s, p + a // b * r, q + a // b * s, -v
    k = (l - q) // s
    t, u = p + k * r, q + k * s
    return (r, s) if 2 * b * u <= n else (t, u)
```

The differences with the standard library version are mostly cosmetic: there are some de-optimizations (like computing `a//b` several times); we deal purely with integers to avoid the `Fraction` unpacking and repacking; some variables are renamed, and we've removed the fast path where a `Fraction` is returned unaltered if its denominator is _already_ at most `l`. The most significant difference is that we've unrolled the first loop-and-a-half of the `while` loop, with the side-effect of shifting the `break` condition to the top of the loop, where it becomes the loop guard. There's also an extra variable `v`, which alternates between values `1` and `-1`. It's not needed for computing the final result, but we'll be making use of it in the statements and proofs below. Finally, the standard library code has no positivity check in the `break` condition to match the `0 < b` check in the loop guard - that check proves to be unnecessary if the fast path is present.

## Proof overview

The main loop of `limit_denominator` is a variant of the extended Euclidean algorithm applied to $m$ and $n$, with an extra termination condition designed to make sure that denominators don't go out of bounds - the moment a full Euclidean step would give us a denominator larger than $\ell$, we halt the loop. After the loop we take a partial Euclidean step, advancing exactly as far as possible without violating the denominator bound.

The main loop, together with that partial Euclidean step, produces candidate approximations $r/s$ and $t/u$ which bracket the target fraction $m/n$, and whose denominators are no greater than the limit $\ell$. With just two additional pieces of information, namely that $ts - ru = ±1$ and that $\ell < s + u$, we can deduce that _all_ fractions strictly between $r/s$ and $t/u$ have denominator greater than $\ell$, hence that either $r/s$ or $t/u$ is a best approximation. The final line of the code then determines which of these two fractions is better and returns it.

In more detail, we'll show in the sections below that after exiting the `while` loop and defining $t$ and $u$, the following five properties hold:

- $(ts - ru)v = 1$
- $0 < s \le \ell$
- $0 < u \le \ell$
- $\ell < s + u$
- $r/s \le m/n < t/u$ if $v=1$, and $t/u < m/n \le r/s$ if $v = -1$

Note that the first three properties make $r/s$ and $t/u$ well-defined as fractions, and moreover both are candidates: reduced, with a positive denominator bounded by $\ell$. We now show that the five properties imply that any fraction $y/z$ strictly between $r/s$ and $t/u$ has $z > \ell$. We have:

$$z = (ts - ru)vz = (tz - yu)vs + (ys - rz)vu.$$

Now $v$ is either $1$ or $-1$. If $v = 1$ then $r/s < t/u$, so $r/s < y/z < t/u$. So $ys - rz$ and $tz - yu$ are positive, hence $(ys - rz)v$ and $(tz - yu)v$ are positive integers. Conversely if $v = -1$ then $t/u < r/s$, so $t/u < y/z < r/s$ and $ys - rz$ and $tz - yu$ are negative. So again $(ys - rz)v$ and $(tz - yu)v$ are positive integers. So in either case, $(tz - yu)v \ge 1$ and $(ys - rz)v \ge 1$, hence
$$z = [(tz-yu)v]s + [(ys - rz)v]u \ge s + u > \ell.$$

At this point $r/s$ and $t/u$ are candidates, there are no candidates strictly between $r/s$ and $t/u$, while $m/n$ _does_ lie between $r/s$ and $t/u$ (and may be equal to $r/s$, but not to $t/u$). It follows that $r/s$ and $t/u$ are the only possible best approximations to $m/n$, and that either $r/s$ or $t/u$ (or possibly both) is a best approximation. The code need only determine which of $r/s$ and $t/u$ is better than the other. The details are given in the "Choosing the best approximation" section below, while the proofs of the five properties that we used above are in the "Details" sections below.

## Details: loop invariants

Both the initial state and the state after every iteration of the `while` loop satisfy the following six invariants:

- $(ps - rq)v = 1$
- $(pn - mq)v = a$
- $(ms - rn)v = b$
- $0 \le b < a$
- $0 \le q$
- $0 < s \le \ell$

These are all easily proved by induction: they hold for the initial state by inspection, and it's easy to see that the way that the variables are updated within the `while` loop keeps all of these properties invariant. Note that at the end of an iteration of the `while` loop, the condition $s \le \ell$ is exactly the condition that the loop was entered (that $q + \lfloor a / b \rfloor s \le \ell$).

That $v$ is either $1$ or $-1$ follows from the first invariant.

## Details: loop termination

For a complete proof of correctness we need to establish that our `while` loop does not run indefinitely - it must terminate after a finite number of iterations. But this follows immediately from the observation that the value of $a$ remains positive and that it strictly decreases at every iteration: at each iteration the new value of $a$ is the old value of $b$, which from our loop invariant $b < a$ is smaller than the old value of $a$.

## Details: after the loop

Here we prove that the five properties mentioned earlier hold.

One of our two possible return values, $r/s$, comes directly from the loop. We construct the second candidate $t/u$ after the loop: $t = p + kr$ and $u = q + ks$, with $k = \lfloor (\ell - q) / s \rfloor$. Think of $t/u$ as the result of taking a _partial_ Euclidean step, as a replacement for the full Euclidean step which would have taken the denominator out of bounds.

That $0 < s \le \ell$ follows directly from the loop invariants. From the definition of $k$ we have $ks \le \ell - q < s + ks$, hence $ks + q \le \ell < s + ks + q$, that is
$$u \le \ell < s + u.$$
That $u$ is positive follows from $s \le \ell < s + u$. So $t/u$ is also a valid candidate approximation.

From construction of $t$ and $u$ and the relation $(ps - rq)v = 1$ we get an analogous relation for $r/s$ and $t/u$:
$$(ts - ru)v = 1.$$

We also define an integer $c = a - kb$, the result of taking the partial Euclidean step that gives $t$ and $u$. It's immediate from the definitions and the earlier loop invariants that:
$$(tn - mu)v = c$$
and
$$bt + cr = m$$
and finally,
$$bu + cs = n.$$

We next prove that $b \le c$. The intuition here is that a full Euclidean step would have given us the nonnegative remainder $a \bmod b$; since we took only a partial step instead, our remainder $c = a - kb$ is the full remainder $a \bmod b$ plus at least one copy of $b$. Concretely, there are two cases. If $0 < b$ then from the failed loop guard $\ell < q + \lfloor a /b \rfloor s$. But $q + ks = u \le \ell$, so $k < \lfloor a/b \rfloor$, from which $c = a - kb \ge a - b\lfloor a/b \rfloor + b \ge b$. Otherwise $b = 0$ and $c = a - kb = a$, and the result is immediate from the positivity of $a$.

Finally, note that $0 < c$. If $0 < b$ this follows from $b \le c$. If $b = 0$ it follows from the positivity of $a$ together with $c = a - kb = a$.

We've now established four of the five properties that were used in the proof overview above. The only property that we're missing is that $m/n$ lies between $r/s$ and $t/u$ (and can be equal to $r/s$, but not to $t/u$). But this follows from the facts that $0 \le b = (ms - rn)v$ and that $0 < c = (tn - mu)v$: if $v = 1$ then these give $r/s \le m/n < t/u$, while if $v = -1$ then $t/u < m/n \le r/s$.

## Choosing the best approximation

By the time we get to the final line of `limit_denominator`, we know that either $r/s$ or $t/u$ is our best approximation; we need to choose the one that's closer to $m/n$. So we want to compare $|m/n - r/s|$ with $|t/u - m/n|$. Scaling up by $nsu$, we need to compare $|(ms - rn)u|$ with $|(tn - mu)s|$. Since $v$ is either plus or minus one, we can insert a factor of $v$ without affecting the absolute value, so we're now comparing $|(ms-rn)vu|$ with $|(tn - mu)vs|$. But from above, $(ms-rn)v = b$ is nonnegative, as is $(tn - mu)v = c$. So the comparison we want to make is
$$bu \le cs.$$
Adding $bu$ to both sides and using the fact that $bu + cs = n$, this is equivalent to the check
$$2bu \le n,$$
which is the check that we use in the code.

In the case that $r/s$ and $t/u$ are equidistant from $m/n$, we have $bu = cs$ (or equivalently $2bu = n$), and $r/s$ is returned. Since $b \le c$ and $c$ is positive, we must have $s = (b/c) u \le u$, so in this case $r/s$ is better than $t/u$ by the tie-break condition, so $r/s$ is a best approximation.

## Uniqueness, and the ambiguous case

From earlier, any best approximation must be either $r/s$ or $t/u$, and at least one of these _is_ a best approximation. Outside one corner case _exactly_ one of these is a best approximation, so there's a unique best approximation to $m/n$ with denominator bounded by $\ell$, and `limit_denominator` returns that best approximation. In this section we explore the corner case where _both_ $r/s$ and $t/u$ are best approximations, and we describe what `limit_denominator` returns in that situation.

So suppose that both $r/s$ and $t/u$ are best approximations. Then each is better than the other, so from the definition of _better_ (including the tie-break condition) they must be equidistant from $m/n$, and we must also have $s \le u$ and $u \le s$, hence $s = u$. From $(ts - ru)v = 1$ and $s = u$ we have $s = u = 1$, so $t = r \pm 1$ and the candidates $r/s$ and $t/u$ are consecutive integers (in some order). Since $m/n$ is equidistant from both, it's a half-integer - a value of the form $w + 1/2$ for some integer $w$. Moreover, since $\ell < s + u$ and $\ell$ is positive, we must have $\ell = 1$.

Conversely, if $m / n$ is a half-integer and $\ell = 1$ then our candidates (denominator bounded by $1$) are simply integers, so it's clear that there are exactly _two_ best approximations to $m / n$, namely
the floor and ceiling of $m / n$.

We call the case where $m / n$ is a half-integer and $\ell = 1$ the **ambiguous** case. The above argument shows that uniqueness fails precisely in the ambiguous case: if $\ell > 1$ or $m / n$ is _not_ a half-integer, then there's a _unique_ best approximation to $m / n$ with denominator bounded by $\ell$.

We now prove that in the ambiguous case, the `limit_denominator` algorithm returns the floor of $m / n$ rather than the ceiling. So suppose again that both $r/s$ and $t/u$ are best approximations. They are then equidistant from $m/n$, so as shown in the previous section $2bu = n$ and `limit_denominator` returns $r/s$. We need to show that $r/s$ is the lower of the two candidates; that is, that $r/s < t/u$, or equivalently that $v = 1$.

We first claim that $q = 0$. From the loop invariants it follows that
$$as + bq = (pn - mq)vs + (ms - rn)vq = (ps - rq)vn = n$$
so we have
$$as + bq = 2bu.$$
Subtracting $bs$ from both sides and rearranging gives
$$(a - b)s = b(2u - q - s).$$
But $b < a$ and $0 < s$ from the loop invariants, so the left-hand side is positive. Since $b$ is nonnegative, $2u - q - s$ must also be positive. As before, we also know that in this ambiguous case $s = u = 1$. So $2u - q - s = 1 - q$ and $1 - q$ is positive, so since $q$ is nonnegative, $q = 0$ is the only possibility. Looking back at the loop, $q$ is nonzero after every iteration of the loop body: the only time it's zero is when the loop hasn't run. And in that initial loop state we do indeed have $v = 1$, which is what we needed.

## Optimizations: removing the $0 < b$ check and avoiding unnecessary `gcd`s

The standard library version of the code has a fast path: if the denominator of the target fraction is _already_ bounded by $\ell$, the code simply returns (a copy of) the target fraction immediately - a moment's thought should convince the reader that $m/n$ is the unique best approximation in this situation.

With the fast path in place, we can simplify the loop guard. Suppose that when the loop guard is evaluated, the $0 < b$ part of the condition is false. We claim that in that case, $n$ is at most $\ell$. For if $0 < b$ is false, then $b = 0$ (we know that $b$ is always nonnegative), the loop exits immediately, we construct $t/u$ as before, and the post-loop equations $bt + cr = m$ and $bu + cs = n$ give $cr = m$ and $cs = n$. So $c$ is a common factor of $m$ and $n$. But since $m$ and $n$ are relatively prime and $c$ is positive, $c = 1$, $m = r$ and $n = s$. But $s \le \ell$, so $n \le \ell$.

So if we've _already_ excluded the case that $n \le \ell$ via the fast path, then the $0 < b$ condition at the top of the loop is always true, and we can drop it. The standard library version of the code does exactly that.

It's worth noting that the simplified listing _does_ require the extra $0 < b$ check, for two reasons. First, the simplified listing does not require that its inputs $m$ and $n$ be relatively prime (though it does require that $n$ is positive). Second, it lacks the fast path. Removing the $0 < b$ check would require both adding a fast path _and_ requiring that $m$ and $n$ be relatively prime.

One other optimization in the standard library version: since we already know that $r/s$ and $t/u$ are in lowest terms, we have no need to construct the returned `Fraction` through the default `Fraction` constructor, which would require a potentially expensive `gcd` computation. We can use the `Fraction._from_coprime_ints` constructor instead.

## Formal proof

In addition to the above proof, a formal, computer-verified proof
of correctness (using Lean) of the algorithm can be found here:

    https://github.com/mdickinson/snippets/tree/1f47f8cc33185c646b679647e88cb24f29fd6a4e/proofs/limit_denominator
