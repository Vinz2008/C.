# Benchmarks compared to other languages

[The benchmarks](https://github.com/Vinz2008/Language-benchmarks)

The benchmark are run on a sytem with a ryzen 5 5600g, 32go of ram and running Manjaro with linux 6.1.60-1.

Finding the 50th Fibonacci number using a recursive function

C. : 124.724s  
C (gcc 13.2.1) : 143.587s  
Rust release (rustc 1.77.1) : 73.132s  
Rust debug (rustc 1.77.1) : 250.724s  
Nim 2.0.2 release :  85.547s  
Nim 2.0.2 debug : 361.758s  
Crystal 1.11.2 : 124.500s
V 0.4.5 : 166.697s   
Swift (swiftc 5.10) : 463.312s
PyPy 7.3.15 : 403.425s  
Php :   
Lua :   
LuaJIT 2.1.1702233742 : 209.25s   
Go 1.21.3 : 140.244s  
Java (Openjdk 22) : 95s  
Python3 3.11.5 : 3228.83s  
Nodejs v21.7.1 : 327.946s  

Finding the factors of 2,000,000,000

C. : 2.716s  
C (gcc 13.2.1) : 3.177s
Rust release (rustc 1.77.1) : 2.712s 
Rust debug (rustc 1.77.1) : 9.577s  
Nim 2.0.2 release : 3.187s  
Swift (swiftc 5.10) : 7.837s
Go 1.22.2 : 3.164s  
LuaJIT 2.1.1702233742 : 2.06s  
V 0.4.5 : 2.727s    
PyPy 7.3.15 : 3.251s 
Nodejs v21.7.1 : 2.786s
Crystal 1.11.2 : 2.714s 
Php 8.3.4 : 15s  
Lua 5.4.6 : 18.02s  
Java (openjdk 22) : ≈3s  
Python 3.11.8 : 113.948s 