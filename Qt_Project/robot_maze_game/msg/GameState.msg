uint8 TILE_WALL=0
uint8 TILE_FLOOR=1

uint8 READY=0
uint8 RUNNING=1
uint8 PAUSED=2
uint8 WON=3
uint8 LOST=4
uint8 ERROR=5

uint32 seed
uint16 columns
uint16 rows
uint8[] tiles

GridPosition player
GridPosition exit
GridPosition[] keys
GridPosition[] enemies

uint32 elapsed_ms
uint32 remaining_ms
int32 score
uint8 collected_keys
uint8 total_keys
uint8 battery
uint8 status
uint8 difficulty
uint32 event_sequence
string event_text
