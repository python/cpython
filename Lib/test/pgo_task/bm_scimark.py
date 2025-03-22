from array import array
import math


class Array2D(object):

    def __init__(self, w, h, data=None):
        self.width = w
        self.height = h
        self.data = array('d', [0]) * (w * h)
        if data is not None:
            self.setup(data)

    def _idx(self, x, y):
        if 0 <= x < self.width and 0 <= y < self.height:
            return y * self.width + x
        raise IndexError

    def __getitem__(self, x_y):
        (x, y) = x_y
        return self.data[self._idx(x, y)]

    def __setitem__(self, x_y, val):
        (x, y) = x_y
        self.data[self._idx(x, y)] = val

    def setup(self, data):
        for y in range(self.height):
            for x in range(self.width):
                self[x, y] = data[y][x]
        return self

    def indexes(self):
        for y in range(self.height):
            for x in range(self.width):
                yield x, y

    def copy_data_from(self, other):
        self.data[:] = other.data[:]


class Random(object):
    MDIG = 32
    ONE = 1
    m1 = (ONE << (MDIG - 2)) + ((ONE << (MDIG - 2)) - ONE)
    m2 = ONE << MDIG // 2
    dm1 = 1.0 / float(m1)

    def __init__(self, seed):
        self.initialize(seed)
        self.left = 0.0
        self.right = 1.0
        self.width = 1.0
        self.haveRange = False

    def initialize(self, seed):

        self.seed = seed
        seed = abs(seed)
        jseed = min(seed, self.m1)
        if (jseed % 2 == 0):
            jseed -= 1
        k0 = 9069 % self.m2
        k1 = 9069 / self.m2
        j0 = jseed % self.m2
        j1 = jseed / self.m2
        self.m = array('d', [0]) * 17
        for iloop in range(17):
            jseed = j0 * k0
            j1 = (jseed / self.m2 + j0 * k1 + j1 * k0) % (self.m2 / 2)
            j0 = jseed % self.m2
            self.m[iloop] = j0 + self.m2 * j1
        self.i = 4
        self.j = 16

    def nextDouble(self):
        I, J, m = self.i, self.j, self.m
        k = m[I] - m[J]
        if (k < 0):
            k += self.m1
        self.m[J] = k

        if (I == 0):
            I = 16
        else:
            I -= 1
        self.i = I

        if (J == 0):
            J = 16
        else:
            J -= 1
        self.j = J

        if (self.haveRange):
            return self.left + self.dm1 * float(k) * self.width
        else:
            return self.dm1 * float(k)

    def RandomMatrix(self, a):
        for x, y in a.indexes():
            a[x, y] = self.nextDouble()
        return a

    def RandomVector(self, n):
        return array('d', [self.nextDouble() for i in range(n)])


def copy_vector(vec):
    # Copy a vector created by Random.RandomVector()
    vec2 = array('d')
    vec2[:] = vec[:]
    return vec2


class ArrayList(Array2D):

    def __init__(self, w, h, data=None):
        self.width = w
        self.height = h
        self.data = [array('d', [0]) * w for y in range(h)]
        if data is not None:
            self.setup(data)

    def __getitem__(self, idx):
        if isinstance(idx, tuple):
            return self.data[idx[1]][idx[0]]
        else:
            return self.data[idx]

    def __setitem__(self, idx, val):
        if isinstance(idx, tuple):
            self.data[idx[1]][idx[0]] = val
        else:
            self.data[idx] = val

    def copy_data_from(self, other):
        for l1, l2 in zip(self.data, other.data):
            l1[:] = l2


def SOR_execute(omega, G, cycles, Array):
    for p in range(cycles):
        for y in range(1, G.height - 1):
            for x in range(1, G.width - 1):
                G[x, y] = (omega * 0.25 * (G[x, y - 1] + G[x, y + 1] + G[x - 1, y]
                                           + G[x + 1, y])
                           + (1.0 - omega) * G[x, y])


def bench_SOR(loops, n, cycles, Array):
    range_it = range(loops)

    for _ in range_it:
        G = Array(n, n)
        SOR_execute(1.25, G, cycles, Array)



def SparseCompRow_matmult(M, y, val, row, col, x, num_iterations):
    range_it = range(num_iterations)

    for _ in range_it:
        for r in range(M):
            sa = 0.0
            for i in range(row[r], row[r + 1]):
                sa += x[col[i]] * val[i]
            y[r] = sa



def bench_SparseMatMult(cycles, N, nz):
    x = array('d', [0]) * N
    y = array('d', [0]) * N

    nr = nz // N
    anz = nr * N
    val = array('d', [0]) * anz
    col = array('i', [0]) * nz
    row = array('i', [0]) * (N + 1)

    row[0] = 0
    for r in range(N):
        rowr = row[r]
        step = r // nr
        row[r + 1] = rowr + nr
        if step < 1:
            step = 1
        for i in range(nr):
            col[rowr + i] = i * step

    return SparseCompRow_matmult(N, y, val, row, col, x, cycles)


def MonteCarlo(Num_samples):
    rnd = Random(113)
    under_curve = 0
    for count in range(Num_samples):
        x = rnd.nextDouble()
        y = rnd.nextDouble()
        if x * x + y * y <= 1.0:
            under_curve += 1
    return float(under_curve) / Num_samples * 4.0


def bench_MonteCarlo(loops, Num_samples):
    range_it = range(loops)

    for _ in range_it:
        MonteCarlo(Num_samples)



def LU_factor(A, pivot):
    M, N = A.height, A.width
    minMN = min(M, N)
    for j in range(minMN):
        jp = j
        t = abs(A[j][j])
        for i in range(j + 1, M):
            ab = abs(A[i][j])
            if ab > t:
                jp = i
                t = ab
        pivot[j] = jp

        if A[jp][j] == 0:
            raise Exception("factorization failed because of zero pivot")

        if jp != j:
            A[j], A[jp] = A[jp], A[j]

        if j < M - 1:
            recp = 1.0 / A[j][j]
            for k in range(j + 1, M):
                A[k][j] *= recp

        if j < minMN - 1:
            for ii in range(j + 1, M):
                for jj in range(j + 1, N):
                    A[ii][jj] -= A[ii][j] * A[j][jj]


def LU(lu, A, pivot):
    lu.copy_data_from(A)
    LU_factor(lu, pivot)


def bench_LU(cycles, N):
    rnd = Random(7)
    A = rnd.RandomMatrix(ArrayList(N, N))
    lu = ArrayList(N, N)
    pivot = array('i', [0]) * N
    range_it = range(cycles)

    for _ in range_it:
        LU(lu, A, pivot)



def int_log2(n):
    k = 1
    log = 0
    while k < n:
        k *= 2
        log += 1
    if n != 1 << log:
        raise Exception("FFT: Data length is not a power of 2: %s" % n)
    return log


def FFT_num_flops(N):
    return (5.0 * N - 2) * int_log2(N) + 2 * (N + 1)


def FFT_transform_internal(N, data, direction):
    n = N // 2
    bit = 0
    dual = 1
    if n == 1:
        return

    logn = int_log2(n)
    if N == 0:
        return
    FFT_bitreverse(N, data)

    # apply fft recursion
    # this loop executed int_log2(N) times
    bit = 0
    while bit < logn:
        w_real = 1.0
        w_imag = 0.0
        theta = 2.0 * direction * math.pi / (2.0 * float(dual))
        s = math.sin(theta)
        t = math.sin(theta / 2.0)
        s2 = 2.0 * t * t
        for b in range(0, n, 2 * dual):
            i = 2 * b
            j = 2 * (b + dual)
            wd_real = data[j]
            wd_imag = data[j + 1]
            data[j] = data[i] - wd_real
            data[j + 1] = data[i + 1] - wd_imag
            data[i] += wd_real
            data[i + 1] += wd_imag
        for a in range(1, dual):
            tmp_real = w_real - s * w_imag - s2 * w_real
            tmp_imag = w_imag + s * w_real - s2 * w_imag
            w_real = tmp_real
            w_imag = tmp_imag
            for b in range(0, n, 2 * dual):
                i = 2 * (b + a)
                j = 2 * (b + a + dual)
                z1_real = data[j]
                z1_imag = data[j + 1]
                wd_real = w_real * z1_real - w_imag * z1_imag
                wd_imag = w_real * z1_imag + w_imag * z1_real
                data[j] = data[i] - wd_real
                data[j + 1] = data[i + 1] - wd_imag
                data[i] += wd_real
                data[i + 1] += wd_imag
        bit += 1
        dual *= 2


def FFT_bitreverse(N, data):
    n = N // 2
    nm1 = n - 1
    j = 0
    for i in range(nm1):
        ii = i << 1
        jj = j << 1
        k = n >> 1
        if i < j:
            tmp_real = data[ii]
            tmp_imag = data[ii + 1]
            data[ii] = data[jj]
            data[ii + 1] = data[jj + 1]
            data[jj] = tmp_real
            data[jj + 1] = tmp_imag
        while k <= j:
            j -= k
            k >>= 1
        j += k


def FFT_transform(N, data):
    FFT_transform_internal(N, data, -1)


def FFT_inverse(N, data):
    n = N / 2
    norm = 0.0
    FFT_transform_internal(N, data, +1)
    norm = 1 / float(n)
    for i in range(N):
        data[i] *= norm


def bench_FFT(loops, N, cycles):
    twoN = 2 * N
    init_vec = Random(7).RandomVector(twoN)
    range_it = range(loops)

    for _ in range_it:
        x = copy_vector(init_vec)
        for i in range(cycles):
            FFT_transform(twoN, x)
            FFT_inverse(twoN, x)



def add_cmdline_args(cmd, args):
    if args.benchmark:
        cmd.append(args.benchmark)


BENCHMARKS = {
    # function name => arguments
    'sor': (bench_SOR, 100, 10, Array2D),
    'sparse_mat_mult': (bench_SparseMatMult, 1000, 50 * 1000),
    'monte_carlo': (bench_MonteCarlo, 100 * 1000,),
    'lu': (bench_LU, 100,),
    'fft': (bench_FFT, 1024, 50),
}


def run_pgo():
    benchmarks = sorted(BENCHMARKS)
    loops = 1
    for bench in benchmarks:
        name = 'scimark_%s' % bench
        func, *args = BENCHMARKS[bench]
        func(loops, *args)
