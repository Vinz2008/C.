# Benchmarks compared to other languages

[The benchmarks](https://github.com/Vinz2008/Language-benchmarks)

The benchmark are run on a sytem with a ryzen 5 5600g, 32go of ram and running Manjaro with linux 6.1.38-1.

Finding the 50th Fibonacci number using a recursive function

C. : 137.688s  
C (gcc 13.1.1) : 146.497s  
Rust release (rustc 1.69.0) : 71.294s  
Rust debug (rustc 1.69.0) : 266.806s  
Nim 1.6.10 release :  27.447s  
Nim 1.6.10 debug : 355.472s  
Swift (swiftc 5.8.1) : 463.776s
Php :   
Lua :   
LuaJIT 2.1.0-beta3 : 288.73s  
Crystal 1.8.2 : 118.306s  
Go 1.20.5 : 133.766s  
Java : 111s  
V 0.4.0 : 180.918s  
Python3 3.11.3 : 3957.103s  
PyPy 7.3.12 : 376.501s  
Nodejs v20.4.0 : 336.720s  

Finding the factors of 2,000,000,000

C. : 6.292s  
C (gcc 13.2.1) : 3.213s 
Rust release (rustc 1.73.0) : 2.726s 
Rust debug (rustc 1.73.0) : 9.742s  
Nim 2.0.0 release : 3.168s  
Swift (swiftc 5.9.1)  : 9.991s
Go 1.21.3 : 3.176s  
LuaJIT 2.1.1697887905 : 2.08s  
V 0.4.2 : 2.817s    
PyPy 7.3.13 : 3.274s 
Nodejs v20.9.0 : 2.809s
Crystal 1.10.1 : 2.719s 
Php 8.2.12 : 17s  
Lua 5.4.6 : 25.76s  
Java (openjdk 20.0.1) : ≈3s  
Python 3.11.5 : 123.618s 