Thinking libevent in C++
============================

Brief
----
    'libevent' does help communication programming a lot! 
    While there are still too many 'callbacks' and 'void*' making things not easy.
    
    So, can we just think communication stuff in OO?
    Yes, here, it is.

Build dependences
-----------------
    (On Fedora) yum install gcc-c++ libstdc++-devel libevent-devel socat.

Build and run
-------------
    $make             #build all samples
    
    $make test_hello  #run the 'libevent hello-world sample' C++ ported.
    
    $make test_cat    #run the 'cat' sample, which implement 'cat' in libevent style.



