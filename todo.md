# TODO

## For initial release

* look into reconnect bug
* share velocity and set the ghosts to update their position based on velocity on frames when a packet is not received
  * could probably bring the send message frequency down a bit (45ms?)
  * maybe have ghosts freeze if an update hasn't happened in a certain amount of time, just so they don't fly off forever if network traffic is really bad
* look into just sending float data as floats instead of ints? would have to figure out precision on serialization
* refactor unnamed namespaces in cpp mod?
* write docs (readme, project anatomy, setup guides for mod and server)
* add license

## Other

* try reconnecting to the server when an error happens instead of only on scene load
* animations?? options to look into:
  * just use animation sequences, send a "best guess" to sync animation state
  * reuse animation bp or player controller, sync whatever data is needed for animations
* also send dream breaker? or at least attach it to ghost sybil when that player is holding it
* name tags or some ui to say who is connected and which level they are in
* make the "message send timeout" calculate correctly?
* have AddGhostData return the list of ghost ids to remove so that BP_PM_Manager.UpdateGhosts doesn't have to recalculate that
  * this could also just be in the message from the server so the client doesn't have to calculate it at all?
* the only info that needs to be shared between threads in the server is the real data, each thread can maintain its own list of "last sent" data
* switch to UDP? would require a lot more management, and we would probably have to make each message contain all data
* add compression and ssl?
* improve setting connect uri, make it runtime configurable?
