
# Details

```

## Internal named timers used by the state engine:

AM::State::m_create_internal_timers() at "ambient3d.cpp"
    will add packet callbacks to reset some timers.

* "PLAYER_YPOS_INTERP_TIMER"  :  Used for interpolating Y position from server.
* "FIXED_TICK_TIMER"          :  Usually used for sending network packets.
* "FIXED_SLOW_TICK_TIMER"     :  Used for unloading things from memory.
* "TIMEOFDAY_INTERP_TIMER"    :  Used for interpolating timeofday from server.

TODO: Write something about every system here.


```
