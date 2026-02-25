/*
 * lidarr.h — Lidarr Custom Script integration
 *
 * When the binary is called by Lidarr as a Custom Script, it receives
 * event info via environment variables.  This module detects that and
 * handles the sync automatically — no shell wrapper needed.
 */

#ifndef LIDARR_H
#define LIDARR_H

/*
 * Check if Lidarr environment variables are present.
 * Returns 1 if running as a Lidarr Custom Script, 0 otherwise.
 */
int lidarr_detect(void);

/*
 * Run the Lidarr handler.  Reads environment variables, determines
 * the album directory, and syncs lyrics.
 *
 * `self_path` is argv[0], used to locate the log file next to the binary.
 * Returns the process exit code (0 = success).
 */
int lidarr_run(const char *self_path);

#endif /* LIDARR_H */
