;
; Copyright (c) 2008-2020 Stefan Krah. All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
;
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
;
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
; OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
; SUCH DAMAGE.
;


(in-package "ACL2")

(include-book "arithmetic/top-with-meta" :dir :system)
(include-book "arithmetic-2/floor-mod/floor-mod" :dir :system)


;; =====================================================================
;;            Proofs for several functions in umodarith.h
;; =====================================================================



;; =====================================================================
;;                          Helper theorems
;; =====================================================================

(defthm elim-mod-m<x<2*m
  (implies (and (<= m x)
                (< x (* 2 m))
                (rationalp x) (rationalp m))
           (equal (mod x m)
                  (+ x (- m)))))

(defthm modaux-1a
  (implies (and (< x m) (< 0 x) (< 0 m)
                (rationalp x) (rationalp m))
           (equal (mod (- x) m)
                  (+ (- x) m))))

(defthm modaux-1b
  (implies (and (< (- x) m) (< x 0) (< 0 m)
                (rationalp x) (rationalp m))
           (equal (mod x m)
                  (+ x m)))
  :hints (("Goal" :use ((:instance modaux-1a
                                   (x (- x)))))))

(defthm modaux-1c
  (implies (and (< x m) (< 0 x) (< 0 m)
                (rationalp x) (rationalp m))
           (equal (mod x m)
                  x)))

(defthm modaux-2a
  (implies (and (< 0 b) (< b m)
                (natp x) (natp b) (natp m)
                (< (mod (+ b x) m) b))
           (equal (mod (+ (- m) b x) m)
                  (+ (- m) b (mod x m)))))

(defthm modaux-2b
  (implies (and (< 0 b) (< b m)
                (natp x) (natp b) (natp m)
                (< (mod (+ b x) m) b))
           (equal (mod (+ b x) m)
                  (+ (- m) b (mod x m))))
  :hints (("Goal" :use (modaux-2a))))

(defthm linear-mod-1
  (implies (and (< x m) (< b m)
                (natp x) (natp b)
                (rationalp m))
         (equal (< x (mod (+ (- b) x) m))
                (< x b)))
  :hints (("Goal" :use ((:instance modaux-1a
                                   (x (+ b (- x))))))))

(defthm linear-mod-2
  (implies (and (< 0 b) (< b m)
                (natp x) (natp b)
                (natp m))
           (equal (< (mod x m)
                     (mod (+ (- b) x) m))
                  (< (mod x m) b))))

(defthm linear-mod-3
  (implies (and (< x m) (< b m)
                (natp x) (natp b)
                (rationalp m))
           (equal (<= b (mod (+ b x) m))
                  (< (+ b x) m)))
  :hints (("Goal" :use ((:instance elim-mod-m<x<2*m
                                   (x (+ b x)))))))

(defthm modaux-2c
  (implies (and (< 0 b) (< b m)
                (natp x) (natp b) (natp m)
                (<= b (mod (+ b x) m)))
           (equal (mod (+ b x) m)
                  (+ b (mod x m))))
  :hints (("Subgoal *1/8''" :use (linear-mod-3))))

(defthmd modaux-2d
  (implies (and (< x m) (< 0 x) (< 0 m)
                (< (- m) b) (< b 0) (rationalp m)
                (<= x (mod (+ b x) m))
                (rationalp x) (rationalp b))
           (equal (+ (- m) (mod (+ b x) m))
                  (+ b x)))
  :hints (("Goal" :cases ((<= 0 (+ b x))))
          ("Subgoal 2'" :use ((:instance modaux-1b
                                        (x (+ b x)))))))

(defthm mod-m-b
  (implies (and (< 0 x) (< 0 b) (< 0 m)
                (< x b) (< b m)
                (natp x) (natp b) (natp m))
           (equal (mod (+ (mod (- x) m) b) m)
                  (mod (- x) b))))


;; =====================================================================
;;                          addmod, submod
;; =====================================================================

(defun addmod (a b m base)
  (let* ((s (mod (+ a b) base))
         (s (if (< s a) (mod (- s m) base) s))
         (s (if (>= s m) (mod (- s m) base) s)))
        s))

(defthmd addmod-correct
  (implies (and (< 0 m) (< m base)
                (< a m) (<= b m)
                (natp m) (natp base)
                (natp a) (natp b))
           (equal (addmod a b m base)
                  (mod (+ a b) m)))
  :hints (("Goal" :cases ((<= base (+ a b))))
          ("Subgoal 2.1'" :use ((:instance elim-mod-m<x<2*m
                                           (x (+ a b)))))))

(defun submod (a b m base)
  (let* ((d (mod (- a b) base))
         (d (if (< a d) (mod (+ d m) base) d)))
        d))

(defthmd submod-aux1
  (implies (and (< a (mod (+ a (- b)) base))
                (< 0 base) (< a base) (<= b base)
                (natp base) (natp a) (natp b))
           (< a b))
  :rule-classes :forward-chaining)

(defthmd submod-aux2
  (implies (and (<= (mod (+ a (- b)) base) a)
                (< 0 base) (< a base) (< b base)
                (natp base) (natp a) (natp b))
           (<= b a))
  :rule-classes :forward-chaining)

(defthmd submod-correct
  (implies (and (< 0 m) (< m base)
                (< a m) (<= b m)
                (natp m) (natp base)
                (natp a) (natp b))
           (equal (submod a b m base)
                  (mod (- a b) m)))
  :hints (("Goal" :cases ((<= base (+ a b))))
          ("Subgoal 2.2" :use ((:instance submod-aux1)))
          ("Subgoal 2.2'''" :cases ((and (< 0 (+ a (- b) m))
                                         (< (+ a (- b) m) m))))
          ("Subgoal 2.1" :use ((:instance submod-aux2)))
          ("Subgoal 1.2" :use ((:instance submod-aux1)))
          ("Subgoal 1.1" :use ((:instance submod-aux2)))))


(defun submod-2 (a b m base)
  (let* ((d (mod (- a b) base))
         (d (if (< a b) (mod (+ d m) base) d)))
        d))

(defthm submod-2-correct
  (implies (and (< 0 m) (< m base)
                (< a m) (<= b m)
                (natp m) (natp base)
                (natp a) (natp b))
           (equal (submod-2 a b m base)
                  (mod (- a b) m)))
  :hints (("Subgoal 2'" :cases ((and (< 0 (+ a (- b) m))
                                     (< (+ a (- b) m) m))))))


;; =========================================================================
;;                         ext-submod is correct
;; =========================================================================

; a < 2*m, b < 2*m
(defun ext-submod (a b m base)
  (let* ((a (if (>= a m) (- a m) a))
         (b (if (>= b m) (- b m) b))
         (d (mod (- a b) base))
         (d (if (< a b) (mod (+ d m) base) d)))
        d))

; a < 2*m, b < 2*m
(defun ext-submod-2 (a b m base)
  (let* ((a (mod a m))
         (b (mod b m))
         (d (mod (- a b) base))
         (d (if (< a b) (mod (+ d m) base) d)))
        d))

(defthmd ext-submod-ext-submod-2-equal
  (implies (and (< 0 m) (< m base)
                (< a (* 2 m)) (< b (* 2 m))
                (natp m) (natp base)
                (natp a) (natp b))
           (equal (ext-submod a b m base)
                  (ext-submod-2 a b m base))))

(defthmd ext-submod-2-correct
  (implies (and (< 0 m) (< m base)
                (< a (* 2 m)) (< b (* 2 m))
                (natp m) (natp base)
                (natp a) (natp b))
           (equal (ext-submod-2 a b m base)
                  (mod (- a b) m))))


;; =========================================================================
;;                            dw-reduce is correct
;; =========================================================================

(defun dw-reduce (hi lo m base)
  (let* ((r1 (mod hi m))
         (r2 (mod (+ (* r1 base) lo) m)))
        r2))

(defthmd dw-reduce-correct
  (implies (and (< 0 m) (< m base)
                (< hi base) (< lo base)
                (natp m) (natp base)
                (natp hi) (natp lo))
           (equal (dw-reduce hi lo m base)
                  (mod (+ (* hi base) lo) m))))

(defthmd <=-multiply-both-sides-by-z
  (implies (and (rationalp x) (rationalp y)
                (< 0 z) (rationalp z))
           (equal (<= x y)
                  (<= (* z x) (* z y)))))

(defthmd dw-reduce-aux1
  (implies (and (< 0 m) (< m base)
                (natp m) (natp base)
                (< lo base) (natp lo)
                (< x m) (natp x))
           (< (+ lo (* base x)) (* base m)))
  :hints (("Goal" :cases ((<= (+ x 1) m)))
          ("Subgoal 1''" :cases ((<= (* base (+ x 1)) (* base m))))
          ("subgoal 1.2" :use ((:instance <=-multiply-both-sides-by-z
                                          (x (+ 1 x))
                                          (y m)
                                          (z base))))))

(defthm dw-reduce-aux2
  (implies (and (< x (* base m))
                (< 0 m) (< m base)
                (natp m) (natp base) (natp x))
           (< (floor x m) base)))

;; This is the necessary condition for using _mpd_div_words().
(defthmd dw-reduce-second-quotient-fits-in-single-word
  (implies (and (< 0 m) (< m base)
                (< hi base) (< lo base)
                (natp m) (natp base)
                (natp hi) (natp lo)
                (equal r1 (mod hi m)))
           (< (floor (+ (* r1 base) lo) m)
              base))
  :hints (("Goal" :cases ((< r1 m)))
          ("Subgoal 1''" :cases ((< (+ lo (* base (mod hi m))) (* base m))))
          ("Subgoal 1.2" :use ((:instance dw-reduce-aux1
                                          (x (mod hi m)))))))


;; =========================================================================
;;                            dw-submod is correct
;; =========================================================================

(defun dw-submod (a hi lo m base)
  (let* ((r (dw-reduce hi lo m base))
         (d (mod (- a r) base))
         (d (if (< a r) (mod (+ d m) base) d)))
        d))

(defthmd dw-submod-aux1
  (implies (and (natp a) (< 0 m) (natp m)
                (natp x) (equal r (mod x m)))
           (equal (mod (- a x) m)
                  (mod (- a r) m))))

(defthmd dw-submod-correct
  (implies (and (< 0 m) (< m base)
                (natp a) (< a m)
                (< hi base) (< lo base)
                (natp m) (natp base)
                (natp hi) (natp lo))
           (equal (dw-submod a hi lo m base)
                  (mod (- a (+ (* base hi) lo)) m)))
  :hints (("Goal" :in-theory (disable dw-reduce)
                  :use ((:instance dw-submod-aux1
                                   (x (+ lo (* base hi)))
                                   (r (dw-reduce hi lo m base)))
                        (:instance dw-reduce-correct)))))


;; =========================================================================
;;                      ANSI C arithmetic for uint64_t
;; =========================================================================

(defun add (a b)
  (mod (+ a b)
       (expt 2 64)))

(defun sub (a b)
  (mod (- a b)
       (expt 2 64)))

(defun << (w n)
  (mod (* w (expt 2 n))
       (expt 2 64)))

(defun >> (w n)
  (floor w (expt 2 n)))

;; join upper and lower half of a double word, yielding a 128 bit number
(defun join (hi lo)
  (+ (* (expt 2 64) hi) lo))


;; =============================================================================
;;                           Fast modular reduction
;; =============================================================================

;; These are the three primes used in the Number Theoretic Transform.
;; A fast modular reduction scheme exists for all of them.
(defmacro p1 ()
  (+ (expt 2 64) (- (expt 2 32)) 1))

(defmacro p2 ()
  (+ (expt 2 64) (- (expt 2 34)) 1))

(defmacro p3 ()
  (+ (expt 2 64) (- (expt 2 40)) 1))


;; reduce the double word number hi*2**64 + lo (mod p1)
(defun simple-mod-reduce-p1 (hi lo)
  (+ (* (expt 2 32) hi) (- hi) lo))

;; reduce the double word number hi*2**64 + lo (mod p2)
(defun simple-mod-reduce-p2 (hi lo)
  (+ (* (expt 2 34) hi) (- hi) lo))

;; reduce the double word number hi*2**64 + lo (mod p3)
(defun simple-mod-reduce-p3 (hi lo)
  (+ (* (expt 2 40) hi) (- hi) lo))


; ----------------------------------------------------------
;      The modular reductions given above are correct
; ----------------------------------------------------------

(defthmd congruence-p1-aux
  (equal (* (expt 2 64) hi)
         (+ (* (p1) hi)
            (* (expt 2 32) hi)
            (- hi))))

(defthmd congruence-p2-aux
  (equal (* (expt 2 64) hi)
         (+ (* (p2) hi)
            (* (expt 2 34) hi)
            (- hi))))

(defthmd congruence-p3-aux
  (equal (* (expt 2 64) hi)
         (+ (* (p3) hi)
            (* (expt 2 40) hi)
            (- hi))))

(defthmd mod-augment
  (implies (and (rationalp x)
                (rationalp y)
                (rationalp m))
           (equal (mod (+ x y) m)
                  (mod (+ x (mod y m)) m))))

(defthmd simple-mod-reduce-p1-congruent
  (implies (and (integerp hi)
                (integerp lo))
           (equal (mod (simple-mod-reduce-p1 hi lo) (p1))
                  (mod (join hi lo) (p1))))
  :hints (("Goal''" :use ((:instance congruence-p1-aux)
                          (:instance mod-augment
                                     (m (p1))
                                     (x (+ (- hi) lo (* (expt 2 32) hi)))
                                     (y (* (p1) hi)))))))

(defthmd simple-mod-reduce-p2-congruent
  (implies (and (integerp hi)
                (integerp lo))
           (equal (mod (simple-mod-reduce-p2 hi lo) (p2))
                  (mod (join hi lo) (p2))))
  :hints (("Goal''" :use ((:instance congruence-p2-aux)
                          (:instance mod-augment
                                     (m (p2))
                                     (x (+ (- hi) lo (* (expt 2 34) hi)))
                                     (y (* (p2) hi)))))))

(defthmd simple-mod-reduce-p3-congruent
  (implies (and (integerp hi)
                (integerp lo))
           (equal (mod (simple-mod-reduce-p3 hi lo) (p3))
                  (mod (join hi lo) (p3))))
  :hints (("Goal''" :use ((:instance congruence-p3-aux)
                          (:instance mod-augment
                                     (m (p3))
                                     (x (+ (- hi) lo (* (expt 2 40) hi)))
                                     (y (* (p3) hi)))))))


; ---------------------------------------------------------------------
;  We need a number less than 2*p, so that we can use the trick from
;  elim-mod-m<x<2*m for the final reduction.
;  For p1, two modular reductions are sufficient, for p2 and p3 three.
; ---------------------------------------------------------------------

;; p1: the first reduction is less than 2**96
(defthmd simple-mod-reduce-p1-<-2**96
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (natp hi) (natp lo))
           (< (simple-mod-reduce-p1 hi lo)
              (expt 2 96))))

;; p1: the second reduction is less than 2*p1
(defthmd simple-mod-reduce-p1-<-2*p1
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (< (join hi lo) (expt 2 96))
                (natp hi) (natp lo))
           (< (simple-mod-reduce-p1 hi lo)
              (* 2 (p1)))))


;; p2: the first reduction is less than 2**98
(defthmd simple-mod-reduce-p2-<-2**98
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (natp hi) (natp lo))
           (< (simple-mod-reduce-p2 hi lo)
              (expt 2 98))))

;; p2: the second reduction is less than 2**69
(defthmd simple-mod-reduce-p2-<-2*69
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (< (join hi lo) (expt 2 98))
                (natp hi) (natp lo))
           (< (simple-mod-reduce-p2 hi lo)
              (expt 2 69))))

;; p3: the third reduction is less than 2*p2
(defthmd simple-mod-reduce-p2-<-2*p2
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (< (join hi lo) (expt 2 69))
                (natp hi) (natp lo))
           (< (simple-mod-reduce-p2 hi lo)
              (* 2 (p2)))))


;; p3: the first reduction is less than 2**104
(defthmd simple-mod-reduce-p3-<-2**104
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (natp hi) (natp lo))
           (< (simple-mod-reduce-p3 hi lo)
              (expt 2 104))))

;; p3: the second reduction is less than 2**81
(defthmd simple-mod-reduce-p3-<-2**81
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (< (join hi lo) (expt 2 104))
                (natp hi) (natp lo))
           (< (simple-mod-reduce-p3 hi lo)
              (expt 2 81))))

;; p3: the third reduction is less than 2*p3
(defthmd simple-mod-reduce-p3-<-2*p3
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (< (join hi lo) (expt 2 81))
                (natp hi) (natp lo))
           (< (simple-mod-reduce-p3 hi lo)
              (* 2 (p3)))))


; -------------------------------------------------------------------------
;      The simple modular reductions, adapted for compiler friendly C
; -------------------------------------------------------------------------

(defun mod-reduce-p1 (hi lo)
  (let* ((y hi)
         (x y)
         (hi (>> hi 32))
         (x (sub lo x))
         (hi (if (> x lo) (+ hi -1) hi))
         (y (<< y 32))
         (lo (add y x))
         (hi (if (< lo y) (+ hi 1) hi)))
        (+ (* hi (expt 2 64)) lo)))

(defun mod-reduce-p2 (hi lo)
  (let* ((y hi)
         (x y)
         (hi (>> hi 30))
         (x (sub lo x))
         (hi (if (> x lo) (+ hi -1) hi))
         (y (<< y 34))
         (lo (add y x))
         (hi (if (< lo y) (+ hi 1) hi)))
        (+ (* hi (expt 2 64)) lo)))

(defun mod-reduce-p3 (hi lo)
  (let* ((y hi)
         (x y)
         (hi (>> hi 24))
         (x (sub lo x))
         (hi (if (> x lo) (+ hi -1) hi))
         (y (<< y 40))
         (lo (add y x))
         (hi (if (< lo y) (+ hi 1) hi)))
        (+ (* hi (expt 2 64)) lo)))


; -------------------------------------------------------------------------
;     The compiler friendly versions are equal to the simple versions
; -------------------------------------------------------------------------

(defthm mod-reduce-aux1
  (implies (and (<= 0 a) (natp a) (natp m)
                (< (- m) b) (<= b 0)
                (integerp b)
                (< (mod (+ b a) m)
                   (mod a m)))
           (equal (mod (+ b a) m)
                  (+ b (mod a m))))
  :hints (("Subgoal 2" :use ((:instance modaux-1b
                                        (x (+ a b)))))))

(defthm mod-reduce-aux2
  (implies (and (<= 0 a) (natp a) (natp m)
                (< b m) (natp b)
                (< (mod (+ b a) m)
                   (mod a m)))
           (equal (+ m (mod (+ b a) m))
                  (+ b (mod a m)))))


(defthm mod-reduce-aux3
  (implies (and (< 0 a) (natp a) (natp m)
                (< (- m) b) (< b 0)
                (integerp b)
                (<= (mod a m)
                    (mod (+ b a) m)))
           (equal (+ (- m) (mod (+ b a) m))
                  (+ b (mod a m))))
  :hints (("Subgoal 1.2'" :use ((:instance modaux-1b
                                           (x b))))
          ("Subgoal 1''" :use ((:instance modaux-2d
                                          (x I))))))


(defthm mod-reduce-aux4
  (implies (and (< 0 a) (natp a) (natp m)
                (< b m) (natp b)
                (<= (mod a m)
                    (mod (+ b a) m)))
           (equal (mod (+ b a) m)
                  (+ b (mod a m)))))


(defthm mod-reduce-p1==simple-mod-reduce-p1
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (natp hi) (natp lo))
           (equal (mod-reduce-p1 hi lo)
                  (simple-mod-reduce-p1 hi lo)))
  :hints (("Goal" :in-theory (disable expt)
                  :cases ((< 0 hi)))
          ("Subgoal 1.2.2'" :use ((:instance mod-reduce-aux1
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 32) hi)))))
          ("Subgoal 1.2.1'" :use ((:instance mod-reduce-aux3
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 32) hi)))))
          ("Subgoal 1.1.2'" :use ((:instance mod-reduce-aux2
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 32) hi)))))
          ("Subgoal 1.1.1'" :use ((:instance mod-reduce-aux4
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 32) hi)))))))


(defthm mod-reduce-p2==simple-mod-reduce-p2
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (natp hi) (natp lo))
           (equal (mod-reduce-p2 hi lo)
                  (simple-mod-reduce-p2 hi lo)))
  :hints (("Goal" :cases ((< 0 hi)))
          ("Subgoal 1.2.2'" :use ((:instance mod-reduce-aux1
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 34) hi)))))
          ("Subgoal 1.2.1'" :use ((:instance mod-reduce-aux3
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 34) hi)))))
          ("Subgoal 1.1.2'" :use ((:instance mod-reduce-aux2
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 34) hi)))))
          ("Subgoal 1.1.1'" :use ((:instance mod-reduce-aux4
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 34) hi)))))))


(defthm mod-reduce-p3==simple-mod-reduce-p3
  (implies (and (< hi (expt 2 64))
                (< lo (expt 2 64))
                (natp hi) (natp lo))
           (equal (mod-reduce-p3 hi lo)
                  (simple-mod-reduce-p3 hi lo)))
  :hints (("Goal" :cases ((< 0 hi)))
          ("Subgoal 1.2.2'" :use ((:instance mod-reduce-aux1
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 40) hi)))))
          ("Subgoal 1.2.1'" :use ((:instance mod-reduce-aux3
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 40) hi)))))
          ("Subgoal 1.1.2'" :use ((:instance mod-reduce-aux2
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 40) hi)))))
          ("Subgoal 1.1.1'" :use ((:instance mod-reduce-aux4
                                             (m (expt 2 64))
                                             (b (+ (- HI) LO))
                                             (a (* (expt 2 40) hi)))))))



