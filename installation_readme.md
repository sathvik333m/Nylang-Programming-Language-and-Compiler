# Install Nylang on Debian or Ubuntu

Download and install the x86-64 Debian package in one command:

```bash
wget -q https://github.com/sathvik333m/Nylang-Programming-Language-and-Compiler/releases/download/v1.0/nylang_1.0_amd64.deb -O nylang_1.0_amd64.deb && sudo apt install ./nylang_1.0_amd64.deb
```

The installed `nylang` command compiles `.ny` files. Install NASM and GCC if they are not already present:

```bash
sudo apt install nasm gcc
```

```bash
nylang hello.ny -o hello
./hello
```
