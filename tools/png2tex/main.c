#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "minigl/texture.h"

void help() {
    printf("Usage: png2tex [options] <input_png> <output_bin>\n");
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -n, --no-alpha Remove alpha channel\n");
}

int main(int argc, char *argv[]) {
    minigl_tex_read_opts_t tex_opts = ((minigl_tex_read_opts_t){0});

    int opt;
    while ((opt = getopt(argc, argv, "nh")) != -1) {
        switch (opt) {
            case 'n':
                tex_opts.force_g8 = true;
                break;

            case 'h':
                help();
                return 0;
            default:
                fprintf(stderr, "Invalid option: -%c\n", opt);
                return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Missing input or output file.\n");
        return 1;
    }

    const char *input_file = argv[optind];
    const char *output_file = argv[optind + 1];

    minigl_tex_t *tex;

    // Check if input file exists
    tex = minigl_tex_read_png(input_file, tex_opts);
    if (tex == NULL) {
        fprintf(stderr, "Error: Input file '%s' not found.\n", input_file);
        return 1;
    }

    // Check if output file is writable
    FILE *f = fopen(output_file, "wb");

    if (f == NULL) {
        fprintf(stderr, "Error: Cannot write to output file '%s'.\n", output_file);
        return 1;
    }

    // Write the structure and the name string
    fwrite(&tex->format, sizeof(minigl_color_fmt_t), 1, f);
    fwrite(&tex->size_x, sizeof(uint32_t), 1, f);
    fwrite(&tex->size_y, sizeof(uint32_t), 1, f);
    for (uint32_t j = 0; j < tex->size_y; j++) {
        switch (tex->format) {
            case MINIGL_COLOR_FMT_G8:
                fwrite(tex->data_g8[j], sizeof(minigl_pixel_g8_t), tex->size_x, f);
                break;
            case MINIGL_COLOR_FMT_GA8:
                fwrite(tex->data_ga8[j], sizeof(minigl_pixel_ga8_t), tex->size_x, f);
                break;
        }
    }
    fclose(f);

    return 0;
}
