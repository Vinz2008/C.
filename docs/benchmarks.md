# Benchmarks compared to other languages

[The benchmarks](https://github.com/Vinz2008/Language-benchmarks)

The benchmark are run on a sytem with a ryzen 5 5600g, 32go of ram and running Manjaro with linux 6.1.38-1.

Finding the 50th Fibonacci number using a recursive function

C. : 230.813s  
C (gcc 13.1.1) : 149.729s  
Rust release (rustc 1.69.0) : 71.294s  
Rust debug (rustc 1.69.0) : 266.806s 
Nim 1.6.10 release :  27.447s  
Nim 1.6.10 debug : 355.472s  
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

C. : 6.334s  
C (gcc 13.1.1) : 3.238s  
Rust release (rustc 1.69.0) : 2.756s  
Rust debug (rustc 1.69.0) : 14.905s  
Nim 1.6.10 release : 3.206s  
Nim 1.6.10 debug : 3.216s  
Php : 17s  
Lua : 20.07s  
LuaJIT 2.1.0-beta3 : 2.09s  
Crystal 1.8.2 : 2.775s 
Go 1.20.5 : 3.208s  
Java (openjdk 20.0.1) : ≈3s  
V 0.4.0 : 2.857s  
Python 3.11.3 : 127.497s  
PyPy 7.3.12 : 3.401s  
Nodejs v20.4.0 : 2.805s  