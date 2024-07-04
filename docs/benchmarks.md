# Benchmarks compared to other languages

[The benchmarks](https://github.com/Vinz2008/Language-benchmarks)

The benchmark are run on a sytem with a ryzen 5 5600g, 32go of ram and running Manjaro with linux 6.1.60-1.

Finding the 50th Fibonacci number using a recursive function

C. no opti : 127.225s  
C. -O3 : 74.849s  
C no opti (gcc 14.1.1) : 147.879s  
C -O3 (gcc 14.1.1) : 43.682s  
Rust release (rustc 1.79.0) : 75.035s  
Rust debug (rustc 1.79.0) : 243.516s  
Nim 2.0.4 release :  89.503s  
Nim 2.0.4 debug : 362.640s  
Crystal 1.11.2 : 191.155s  
V 0.4.6 : 187.359s   
Swift (swiftc 5.10.1) : 473.061s  
PyPy 7.3.16 : 346.977s  
Php 8.3.8 : 2258s  
Lua 5.4.7 : 2002.52s 
LuaJIT 2.1.1702233742 : 387.21s   
Go 1.22.4 : 144.123s  
Java (Openjdk 22) : 98s  
Python3 3.12.3 : 3830.016s  
Nodejs v22.3.0 : 323.370s  

Finding the factors of 2,000,000,000

C. no opti : 2.784s  
C. -O3 : 2.764s  
C no opti (gcc 14.1.1) : 3.260s  
C -O3 (gcc 14.1.1) : 3.234s  
Rust release (rustc 1.79.0) : 2.767s  
Rust debug (rustc 1.79.0) : 14.149s  
Nim 2.0.4 release : 3.275s  
Swift (swiftc 5.10.1) : 8.567s  
Go 1.22.4 : 3.250s  
LuaJIT 2.1.1716656478 : 2.126s  
V 0.4.6 : 2.864s    
PyPy 7.3.16 : 3.391s  
Nodejs v22.3.0 : 2.880s  
Crystal 1.12.2 : 2.768s  
Php 8.3.8 : 18.128s  
Lua 5.4.6 : 19.013s  
Java (openjdk 22) : 3.121s  
Python 3.12.4 : 152.603s 