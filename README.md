cocaine_lru
===========
## What is this about?
This is a simple implementation of an LRU caching system using cocaine infraestructure. When a request is made, cocaine will span as many workers as needed.

Everything is stored in the memory, no flushing to the disks happen at any time. Given the distributed nature of cocaine, the scalability of this system it has no limits.

## Requirements
In order to compile an use the system, boost, cocaine native framework and elliptics libraries are needed. The code uses C++11.

## How does it work?
The client sends an exec scrip request through HTTP. The server receives it, and pass it to elliptics using fastcgi protocol. Then elliptics executes the script, which brings up a worker, if it's needed. In all the workers are busy, and no more workers can be launched, the request will be enqueued, and executed as soon as one of the workers frees up.