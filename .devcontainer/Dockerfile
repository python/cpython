FROM docker.io/library/fedora:37

ENV CC=clang

RUN dnf -y update && \
    dnf -y install git clang dnf-plugins-core && \
    dnf -y builddep python3
