# TODO

## General Plan

### Phase 1

Goal: share current position and render other players as simple ghosts

#### Client ping

* every .016 seconds, regardless of frame rate
* send current zone/position
  ```json
  {
    "zone": "Zone_Caves",
    "position": {
      "x": 140,
      "y": -32,
      "z": 2657
    }
  }
  ```
* round position to the nearest int?
* optimizations
  * leave off stuff that hasn't changed since last update sent
  * if nothing has changed, no ping? idk
  * shorter keys? `"z"` for zone, `"p"` for position

#### Server ping

* send current zone/position of every other player
    to reduce message size, you could only send fields that have changed?
    or only send the zone for players in a different zone
    {1:{x:0,y:0,z:0,l:"ZONE_Exterior"},2:{l:"Zone_Caves"}}
    since zones are the larges part of the message, could also group by zone
    {"ZONE_Exterior":{1:{x:0,y:0,z:0}},"Zone_Caves":{2:{}}}
  client creates a ghost actor for every other player that is in the same zone
    mod actor manages ghost actors
    ghost actor shows up as a shrunk down white aura? idk doesn't matter but something like that
    mod actor tick updates each ghost actor's position, creates/deletes ghost actors that changed zones

### Phase 2

Goal: share orientation and animation state and render ghosts as sybil

### Phase 3

Goal: game modes?

### Phase 4

Goal: proximity chat???
