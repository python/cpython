# pkg-config overrides for RHEL 7 and CentOS 7

RHEL 7 and CentOS 7 do not provide pkg-config `.pc` files for Tcl/Tk. The
 OpenSSL 1.1.1 pkg-config file is named `openssl11.pc` and not picked up
 by Python's `configure` script.

To build Python with system Tcl/Tk libs and OpenSSL 1.1 package, first
install the developer packages and the `pkgconfig` package with `pkg-config`
command.

```shell
sudo yum install pkgconfig 'tcl-devel >= 8.5.12' 'tk-devel >= 8.5.12' openssl11-devel
```

The run `configure` with `PKG_CONFIG_PATH` environment variable.

```shell
PKG_CONFIG_PATH=Misc/rhel7 ./configure -C
```
