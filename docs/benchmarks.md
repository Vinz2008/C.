# Benchmarks compared to other languages

[The benchmarks](https://github.com/Vinz2008/Language-benchmarks)

The benchmark are run on a sytem with a ryzen 5 5600g, 32go of ram and running Manjaro with linux 6.1.60-1.

Finding the 50th Fibonacci number using a recursive function

C. : 136.970s  
C (gcc 13.2.1) : 145.179s  
Rust release (rustc 1.73.0) : 73.856s  
Rust debug (rustc 1.73.0) : 237.043s  
Nim 2.0.0 release :  40.671s  
Nim 2.0.0 debug : 304.621s  
Crystal 1.10.1 : 112.819s
V 0.4.2 : 178.815s   
Swift (swiftc 5.9.1) : 471.683s
PyPy 7.3.13 : 377.04s  
Php :   
Lua :   
LuaJIT 2.1.1697887905 : 266.69s   
Go 1.21.3 : 142.718s  
Java (Openjdk 21) : 95s  
Python3 3.11.5 : 3292.322s  
Nodejs v20.9.0 : 332.740s  

Finding the factors of 2,000,000,000

C. : 2.772s  
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