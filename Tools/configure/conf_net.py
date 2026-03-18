"""conf_net — Network configuration checks.

Probes for nsl/socket/network libraries and --with-libs; handles
--enable-ipv6 with platform-specific IPv6 stack type detection;
checks Linux CAN socket support; checks netdb and socket functions;
validates getaddrinfo (including broken-resolver bug check); probes
gethostbyname_r (3, 5, or 6 argument variants); and checks socklen_t.

Consolidates networking code previously scattered across conf_sharedlib,
conf_syslibs, conf_probes, conf_math, and conf_checks.
"""

from __future__ import annotations

import pyconf

WITH_LIBS = pyconf.arg_with(
    "libs",
    metavar="'lib1 ...'",
    help="link against additional libs (default is no)",
)
ENABLE_IPV6 = pyconf.arg_enable(
    "ipv6",
    help="enable ipv6 (with ipv4) support (default is yes if supported)",
)


# ---------------------------------------------------------------------------
# From conf_sharedlib
# ---------------------------------------------------------------------------


def check_network_libs(v):
    """Check for nsl, socket, network libraries and --with-libs."""
    if pyconf.check_lib("nsl", "t_open"):
        v.LIBS = f"-lnsl {v.LIBS}"
    if pyconf.check_lib("socket", "socket"):
        v.LIBS = f"-lsocket {v.LIBS}"
    if f"{v.ac_sys_system}/{v.ac_sys_release}".startswith("Haiku"):
        if pyconf.check_lib("network", "socket"):
            v.LIBS = f"-lnetwork {v.LIBS}"

    pyconf.checking("for --with-libs")
    libs_val = WITH_LIBS.value
    if libs_val is not None and libs_val != "no":
        pyconf.result(libs_val)
        v.LIBS = f"{libs_val} {v.LIBS}"
    else:
        pyconf.result("no")


# ---------------------------------------------------------------------------
# From conf_syslibs
# ---------------------------------------------------------------------------


def check_ipv6(v):
    """Handle --enable-ipv6 and IPv6 stack type detection."""
    pyconf.checking("if --enable-ipv6 is specified")

    if ENABLE_IPV6.is_no():
        pyconf.result("no")
        v.ipv6 = False
    elif ENABLE_IPV6.is_yes():
        pyconf.result("yes")
        pyconf.define("ENABLE_IPV6")
        v.ipv6 = True
    else:
        # Auto-detect
        if pyconf.compile_check(
            preamble="#include <sys/types.h>\n#include <sys/socket.h>",
            body="int domain = AF_INET6;",
        ):
            v.ipv6 = True
        else:
            v.ipv6 = False

        if v.ac_sys_system == "WASI":
            v.ipv6 = False

        pyconf.result(v.ipv6)

        if v.ipv6:
            pyconf.checking("if RFC2553 API is available")
            if pyconf.compile_check(
                preamble="#include <sys/types.h>\n#include <netinet/in.h>",
                body="struct sockaddr_in6 x; int y = x.sin6_scope_id;",
            ):
                pyconf.result("yes")
            else:
                pyconf.result("no")
                v.ipv6 = False

        if v.ipv6:
            pyconf.define("ENABLE_IPV6")

    # IPv6 stack type detection
    ipv6type = "unknown"
    ipv6lib = "none"
    ipv6libdir = ""
    ipv6trylibc = False
    BASECFLAGS = v.BASECFLAGS

    if v.ipv6 and not pyconf.cross_compiling:
        for i in (
            "inria",
            "kame",
            "linux-glibc",
            "linux-inet6",
            "solaris",
            "toshiba",
            "v6d",
            "zeta",
        ):
            if i == "kame" and pyconf.check_define("netinet/in.h", "__KAME__"):
                ipv6type = i
                ipv6lib = "inet6"
                ipv6libdir = "/usr/local/v6/lib"
                ipv6trylibc = True
            elif i == "linux-glibc" and pyconf.check_define(
                "features.h", "__GLIBC__"
            ):
                ipv6type = i
                ipv6trylibc = True
            elif i == "linux-inet6" and pyconf.path_is_dir("/usr/inet6"):
                ipv6type = i
                ipv6lib = "inet6"
                ipv6libdir = "/usr/inet6/lib"
                BASECFLAGS = "-I/usr/inet6/include " + BASECFLAGS
            elif i == "solaris" and pyconf.path_is_file("/etc/netconfig"):
                netconfig_content = pyconf.read_file("/etc/netconfig")
                if "tcp6" in netconfig_content:
                    ipv6type = i
                    ipv6trylibc = True
            elif i == "inria" and pyconf.check_define(
                "netinet/in.h", "IPV6_INRIA_VERSION"
            ):
                ipv6type = i
            elif i == "toshiba" and pyconf.check_define(
                "sys/param.h", "_TOSHIBA_INET6"
            ):
                ipv6type = i
                ipv6lib = "inet6"
                ipv6libdir = "/usr/local/v6/lib"
            elif i == "v6d" and pyconf.check_define(
                "/usr/local/v6/include/sys/v6config.h", "__V6D__"
            ):
                ipv6type = i
                ipv6lib = "v6"
                ipv6libdir = "/usr/local/v6/lib"
                BASECFLAGS = "-I/usr/local/v6/include " + BASECFLAGS
            elif i == "zeta" and pyconf.check_define(
                "sys/param.h", "_ZETA_MINAMI_INET6"
            ):
                ipv6type = i
                ipv6lib = "inet6"
                ipv6libdir = "/usr/local/v6/lib"
            if ipv6type != "unknown":
                break
        pyconf.checking("ipv6 stack type")
        pyconf.result(ipv6type)

    v.BASECFLAGS = BASECFLAGS  # may have inet6 include path appended

    if v.ipv6 and ipv6lib != "none":
        pyconf.checking("ipv6 library")
        libpath = pyconf.path_join([ipv6libdir, f"lib{ipv6lib}.a"])
        if ipv6libdir and pyconf.path_is_file(libpath):
            v.LIBS = f"-L{ipv6libdir} -l{ipv6lib} {v.LIBS}"
            pyconf.result(f"lib{ipv6lib}")
        elif ipv6trylibc:
            pyconf.result("libc")
        else:
            pyconf.fatal(
                f"No {ipv6lib} library found; cannot continue.\n"
                f"You need to fetch lib{ipv6lib}.a from appropriate ipv6 kit."
            )


def check_can_sockets(v):
    """Check CAN socket support (CAN_RAW_FD_FRAMES, CAN_RAW_JOIN_FILTERS)."""
    pyconf.checking("CAN_RAW_FD_FRAMES")
    if pyconf.compile_check(
        program="#include <linux/can/raw.h>\n"
        "int main(void) { int can_raw_fd_frames = CAN_RAW_FD_FRAMES; return 0; }\n"
    ):
        pyconf.result("yes")
        pyconf.define(
            "HAVE_LINUX_CAN_RAW_FD_FRAMES",
            1,
            "Define if compiling using Linux 3.6 or later.",
        )
    else:
        pyconf.result("no")

    pyconf.checking("for CAN_RAW_JOIN_FILTERS")
    if pyconf.compile_check(
        program="#include <linux/can/raw.h>\n"
        "int main(void) { int can_raw_join_filters = CAN_RAW_JOIN_FILTERS; return 0; }\n"
    ):
        pyconf.result("yes")
        pyconf.define(
            "HAVE_LINUX_CAN_RAW_JOIN_FILTERS",
            1,
            "Define if compiling using Linux 4.1 or later.",
        )
    else:
        pyconf.result("no")


# ---------------------------------------------------------------------------
# From conf_probes
# ---------------------------------------------------------------------------


def _check_netdb_func(func):
    pyconf.check_func(func, includes=["netdb.h"])


def _check_socket_func(func):
    pyconf.check_func(
        func,
        includes=[
            "sys/types.h",
            "sys/socket.h",
            "netinet/in.h",
            "arpa/inet.h",
        ],
    )


def check_netdb_socket_funcs(v):
    # ---------------------------------------------------------------------------
    # Netdb / socket function checks
    # PY_CHECK_NETDB_FUNC / PY_CHECK_SOCKET_FUNC helper wrappers
    # ---------------------------------------------------------------------------

    for f in (
        "hstrerror",
        "getservbyname",
        "getservbyport",
        "gethostbyname",
        "gethostbyaddr",
        "getprotobyname",
    ):
        _check_netdb_func(f)

    for f in (
        "inet_aton",
        "inet_ntoa",
        "inet_pton",
        "getpeername",
        "getsockname",
        "accept",
        "bind",
        "connect",
        "listen",
        "recvfrom",
        "sendto",
        "setsockopt",
        "socket",
    ):
        _check_socket_func(f)

    # setgroups — in unistd.h on some systems, grp.h on others
    pyconf.check_func(
        "setgroups", includes=["unistd.h"], conditional_headers=["HAVE_GRP_H"]
    )


def check_getaddrinfo(v):
    # ---------------------------------------------------------------------------
    # getaddrinfo probe (full run-test)
    # ---------------------------------------------------------------------------

    pyconf.checking("for getaddrinfo")
    ac_cv_func_getaddrinfo = pyconf.link_check(
        preamble="""
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <stdio.h>
    """,
        body="getaddrinfo(NULL, NULL, NULL, NULL);",
        cache_var="ac_cv_func_getaddrinfo",
    )
    pyconf.result("yes" if ac_cv_func_getaddrinfo else "no")

    if ac_cv_func_getaddrinfo:
        GETADDRINFO_TEST = r"""
    #include <stdio.h>
    #include <sys/types.h>
    #include <netdb.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <netinet/in.h>

    int main(void)
    {
      int passive, gaierr, inet4 = 0, inet6 = 0;
      struct addrinfo hints, *ai, *aitop;
      char straddr[INET6_ADDRSTRLEN], strport[16];

      for (passive = 0; passive <= 1; passive++) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = passive ? AI_PASSIVE : 0;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        if ((gaierr = getaddrinfo(NULL, "54321", &hints, &aitop)) != 0) {
          (void)gai_strerror(gaierr);
          goto bad;
        }
        for (ai = aitop; ai; ai = ai->ai_next) {
          if (ai->ai_addr == NULL ||
              ai->ai_addrlen == 0 ||
              getnameinfo(ai->ai_addr, ai->ai_addrlen,
                          straddr, sizeof(straddr), strport, sizeof(strport),
                          NI_NUMERICHOST|NI_NUMERICSERV) != 0) {
            goto bad;
          }
          switch (ai->ai_family) {
          case AF_INET:
            if (strcmp(strport, "54321") != 0) goto bad;
            if (passive) {
              if (strcmp(straddr, "0.0.0.0") != 0) goto bad;
            } else {
              if (strcmp(straddr, "127.0.0.1") != 0) goto bad;
            }
            inet4++;
            break;
          case AF_INET6:
            if (strcmp(strport, "54321") != 0) goto bad;
            if (passive) {
              if (strcmp(straddr, "::") != 0) goto bad;
            } else {
              if (strcmp(straddr, "::1") != 0) goto bad;
            }
            inet6++;
            break;
          case AF_UNSPEC:
            goto bad;
          default:
            break;
          }
        }
        freeaddrinfo(aitop);
        aitop = NULL;
      }
      if (!(inet4 == 0 || inet4 == 2)) goto bad;
      if (!(inet6 == 0 || inet6 == 2)) goto bad;
      if (aitop) freeaddrinfo(aitop);
      return 0;
     bad:
      if (aitop) freeaddrinfo(aitop);
      return 1;
    }
    """
        pyconf.checking("getaddrinfo bug")
        if pyconf.cross_compiling:
            if v.ac_sys_system in ("Linux-android", "iOS"):
                ac_cv_buggy_getaddrinfo = False
            elif v.enable_ipv6 != "":
                ac_cv_buggy_getaddrinfo = (
                    "no -- configured with --(en|dis)able-ipv6"
                )
            else:
                ac_cv_buggy_getaddrinfo = True
        else:
            ok = pyconf.run_check(GETADDRINFO_TEST)
            ac_cv_buggy_getaddrinfo = not ok
        pyconf.result(ac_cv_buggy_getaddrinfo)
    else:
        ac_cv_buggy_getaddrinfo = True

    if not ac_cv_func_getaddrinfo or ac_cv_buggy_getaddrinfo is True:
        if v.ipv6:
            pyconf.error(
                "You must get working getaddrinfo() function "
                'or pass the "--disable-ipv6" option to configure.'
            )
    else:
        pyconf.define(
            "HAVE_GETADDRINFO",
            1,
            "Define if you have the getaddrinfo function.",
        )

    pyconf.check_func("getnameinfo")


# ---------------------------------------------------------------------------
# From conf_math
# ---------------------------------------------------------------------------


def check_gethostbyname_r(v):
    # ---------------------------------------------------------------------------
    # gethostbyname_r (3, 5, or 6 argument variants)
    # ---------------------------------------------------------------------------

    pyconf.define_template(
        "HAVE_GETHOSTBYNAME_R",
        "Define this if you have some version of gethostbyname_r()",
    )

    if pyconf.check_func("gethostbyname_r"):
        pyconf.define("HAVE_GETHOSTBYNAME_R")
        old_cflags = v.CFLAGS
        v.CFLAGS = f"{v.CFLAGS} {v.MY_CPPFLAGS} {v.MY_THREAD_CPPFLAGS} {v.MY_CFLAGS}".strip()

        pyconf.checking("gethostbyname_r with 6 args")
        if pyconf.compile_check(
            preamble="#include <netdb.h>",
            body="""
        char *name;
        struct hostent *he, *res;
        char buffer[2048];
        int buflen = 2048;
        int h_errnop;
        (void) gethostbyname_r(name, he, buffer, buflen, &res, &h_errnop)
    """,
        ):
            pyconf.define("HAVE_GETHOSTBYNAME_R")
            pyconf.define(
                "HAVE_GETHOSTBYNAME_R_6_ARG",
                1,
                "Define this if you have the 6-arg version of gethostbyname_r().",
            )
            pyconf.result("yes")
        else:
            pyconf.result("no")
            pyconf.checking("gethostbyname_r with 5 args")
            if pyconf.compile_check(
                preamble="#include <netdb.h>",
                body="""
        char *name;
        struct hostent *he;
        char buffer[2048];
        int buflen = 2048;
        int h_errnop;
        (void) gethostbyname_r(name, he, buffer, buflen, &h_errnop)
    """,
            ):
                pyconf.define("HAVE_GETHOSTBYNAME_R")
                pyconf.define(
                    "HAVE_GETHOSTBYNAME_R_5_ARG",
                    1,
                    "Define this if you have the 5-arg version of gethostbyname_r().",
                )
                pyconf.result("yes")
            else:
                pyconf.result("no")
                pyconf.checking("gethostbyname_r with 3 args")
                if pyconf.compile_check(
                    preamble="#include <netdb.h>",
                    body="""
        char *name;
        struct hostent *he;
        struct hostent_data data;
        (void) gethostbyname_r(name, he, &data);
    """,
                ):
                    pyconf.define("HAVE_GETHOSTBYNAME_R")
                    pyconf.define(
                        "HAVE_GETHOSTBYNAME_R_3_ARG",
                        1,
                        "Define this if you have the 3-arg version of gethostbyname_r().",
                    )
                    pyconf.result("yes")
                else:
                    pyconf.result("no")

        v.CFLAGS = old_cflags
    else:
        pyconf.check_func("gethostbyname")

    v.export("HAVE_GETHOSTBYNAME_R_6_ARG")
    v.export("HAVE_GETHOSTBYNAME_R_5_ARG")
    v.export("HAVE_GETHOSTBYNAME_R_3_ARG")
    v.export("HAVE_GETHOSTBYNAME_R")
    v.export("HAVE_GETHOSTBYNAME")


# ---------------------------------------------------------------------------
# From conf_checks
# ---------------------------------------------------------------------------


def check_socklen_t(v):
    # ---------------------------------------------------------------------------
    # socklen_t
    # ---------------------------------------------------------------------------

    if not pyconf.check_type(
        "socklen_t", headers=["sys/types.h", "sys/socket.h"]
    ):
        pyconf.define(
            "socklen_t",
            "int",
            "Define to 'int' if <sys/socket.h> does not define.",
        )
