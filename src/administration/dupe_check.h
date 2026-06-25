/**
 \file dupe_check.h - a part of the Bylins engine.
 \brief issue.interpreter-cleaning (Bucket 2): multi-play / duplicate-login detection (same IP / same
        email), moved out of the command interpreter. Account-connection policy.
*/
#pragma once

class DescriptorData;

// Same-IP multi-play gate. autocheck=true -> silent (no player message). Returns 0 if blocked.
int check_dupes_host(DescriptorData *d, bool autocheck = false);
// Same-email simultaneous-login gate. Returns 0 if blocked.
int check_dupes_email(DescriptorData *d);
