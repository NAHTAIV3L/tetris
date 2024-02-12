#include <string.h>

#define NOB_IMPLEMENTATION
#include "./nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
  
    const char * const program = nob_shift_args(&argc, &argv);
  
    Nob_Cmd cmd = {0};

    bool debug = false;
    bool xlib_render = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "debug") == 0) {
          debug = true;
        }
        if (strcmp(argv[i], "xlib-render") == 0) {
            xlib_render = true;
        }
    }
    nob_cmd_append(&cmd, "cc");
    nob_cmd_append(&cmd, "-Wall", "-g");
    nob_cmd_append(&cmd, "-lX11", "-lpthread", "-lfreetype", "-lfontconfig", "-lXft", "-lGL", "-lGLEW");
    nob_cmd_append(&cmd, "-I/usr/include/freetype2", "-I/usr/include/harfbuzz", "-I/usr/include/glib-2.0", "-I/usr/lib64/glib-2.0/include");
    nob_cmd_append(&cmd, "-o", "tetris");
    nob_cmd_append(&cmd, "tetris.c", "xftfont.c", "shader.c", "la.c", "main.c", "render.c", "free_glyph.c");
    if (debug) nob_cmd_append(&cmd, "-DTETRIS_DEBUG");
    if (xlib_render) nob_cmd_append(&cmd, "-DXLIB_RENDER");
    if (!nob_cmd_run_sync(cmd)) return 1;
    cmd.count = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "run") == 0) {
        cmd.count = 0;
        nob_cmd_append(&cmd, "./tetris");
        if (!nob_cmd_run_sync(cmd)) return 1;
        }
    }
    return 0;
}

