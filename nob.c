#include <string.h>

#define NOB_IMPLEMENTATION
#include "./nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
  
    const char * const program = nob_shift_args(&argc, &argv);
  
    Nob_Cmd cmd = {0};

    bool fdebug = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "debug") == 0) {
          nob_cmd_append(&cmd, "cc");
          nob_cmd_append(&cmd, "-Wall", "-g");
          nob_cmd_append(&cmd, "-lX11", "-lpthread", "-lfontconfig", "-lXft");
          nob_cmd_append(&cmd, "-I/usr/include/freetype2", "-DTETRIS_DEBUG");
          nob_cmd_append(&cmd, "-o", "tetris");
          nob_cmd_append(&cmd, "tetris.c", "xftfont.c");
          if (!nob_cmd_run_sync(cmd)) return 1;
          cmd.count = 0;
          fdebug = true;
        }
    }
    if (!fdebug) {
        nob_cmd_append(&cmd, "cc");
        nob_cmd_append(&cmd, "-Wall", "-g");
        nob_cmd_append(&cmd, "-lX11", "-lpthread", "-lfontconfig", "-lXft");
        nob_cmd_append(&cmd, "-I/usr/include/freetype2");
        nob_cmd_append(&cmd, "-o", "tetris");
        nob_cmd_append(&cmd, "tetris.c", "xftfont.c");
        if (!nob_cmd_run_sync(cmd)) return 1;
        cmd.count = 0;
    }

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "run") == 0) {
        cmd.count = 0;
        nob_cmd_append(&cmd, "./tetris");
        if (!nob_cmd_run_sync(cmd)) return 1;
        }
    }
    return 0;
}

