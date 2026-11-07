#pragma once

// VU1 execution options a game's runtime chooses. Process-wide; set before
// the game starts.

// XGKICK copies the kicked GIF packet when the kick issues and only keeps the
// transfer's timing, instead of reading a qword from VU memory every two
// cycles as the hardware does. Same cycle count, far less work, but a program
// that rewrites a buffer while it is still being sent gets the mix the
// hardware would only with the per-qword path, so a game enables this once
// PS2X_XGKICK_VERIFY=1 has shown it never does that. PS2X_XGKICK_SYNC=1 keeps
// the per-qword path regardless.
void ps2Vu1SetXgkickCopyAtIssue(bool enabled);
bool ps2Vu1XgkickCopyAtIssue();
