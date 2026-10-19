// Scene definitions for ble_spam
// Each scene maps to a handler function

// Scene 0: Submenu
ADD_SCENE(ble_spam, submenu, Submenu)
// Scene 1: Running
ADD_SCENE(ble_spam, running, Running)
// Scene 2: Text input
ADD_SCENE(ble_spam, text_input, TextInput)
// Scene 3: Walk scan
ADD_SCENE(ble_spam, walk_scan, WalkScan)
// Scene 4: Walk detail
ADD_SCENE(ble_spam, walk_detail, WalkDetail)
// Scene 5: Auto walk
ADD_SCENE(ble_spam, auto_walk, AutoWalk)
// Scene 6: Tracker scan
ADD_SCENE(ble_spam, tracker_scan, TrackerScan)
// Scene 7: Tracker geiger
ADD_SCENE(ble_spam, tracker_geiger, TrackerGeiger)
// Scene 8: Race detector
ADD_SCENE(ble_spam, race_detector, RaceDetector)