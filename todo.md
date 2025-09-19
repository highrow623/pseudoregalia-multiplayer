# TODO

* look into reconnect bug
* share velocity and set the ghosts to update their position based on velocity on frames when a packet is not received
  * could probably bring the send message frequency down a bit (45ms?)
  * maybe have ghosts freeze if an update hasn't happened in a certain amount of time, just so they don't fly off forever if network traffic is really bad
* write docs (readme, project anatomy, setup guides)
* animations?? maybe reuse animation bp
* also send dream breaker? or at least attach it to ghost sybil when that player is holding it
* make the message send timeout calculate correctly?

Optimizations

* the only info that needs to be shared between threads in the server is the real data, each thread can maintain its own list of "last sent" data
* switch to UDP? would require a lot more management, and we would probably have to make each message contain all data
