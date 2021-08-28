#define HS_IMPL
#define HS_NUKLEAR
#define HS_SFD
#include "hs/hs_graphics.h"

hs_game_data gd = {
        .width = 960,
        .height = 540
};

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

hs_key* escape = hs_key_init(GLFW_KEY_ESCAPE);

uint32_t popup_menu_up = 1;
uint32_t popup_new_up = 0;

struct nk_context *ctx;
struct nk_glfw glfw = {0};

sfd_Options anders_tale_room = {
  .title        = "Johan, you may now choose a Anders Tale Room file",
  .filter_name  = "Anders tale room",
  .filter       = "*.aroom",
};

static inline void init()
{
        hs_init(&gd, "Anders tale room editor (mfw Johan anderstaler istendefor å game)", NULL, 0);

        ctx = nk_glfw3_init(&glfw, gd.window, NK_GLFW3_INSTALL_CALLBACKS);
        /* Load Fonts: if none of these are loaded a default font will be used  */
        /* Load Cursor: if you uncomment cursor loading please hide the cursor */
        {
                struct nk_font_atlas *atlas;
                nk_glfw3_font_stash_begin(&glfw, &atlas);
                nk_glfw3_font_stash_end(&glfw);
        }
}

static inline void popup_start()
{

        if (nk_begin(ctx, "Main Menu", nk_rect(gd.width/2 - 100, gd.height/2 - 70, 200, 140),
                    NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {

                nk_layout_row_dynamic(ctx, 30, 1);
                if (nk_button_label(ctx, "New room")) {
                        popup_new_up = true;
                        popup_menu_up = false;
                }

                nk_layout_row_dynamic(ctx, 30, 1);
                if (nk_button_label(ctx, "load room")) {
                        const char *filename = sfd_open_dialog(&anders_tale_room);
                        if (filename) {
                                printf("Got file: '%s'\n", filename);
                        } else {
                                printf("load canceled\n");
                        }
                }

                if (nk_button_label(ctx, "Quit without saving")) {
                        exit(0);
                }
        }
        nk_end(ctx);
}

static inline void popup_new_room()
{

        if (nk_begin(ctx, "Create new room", nk_rect(gd.width/2 - 100, gd.height/2 - 85, 200, 170),
                    NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {

                nk_layout_row_dynamic(ctx, 30, 1);
                static int new_room_width = 0;
                nk_property_int(ctx, "Room width:", 0, &new_room_width, 100, 1, 1);
                static int new_room_height = 0;
                nk_property_int(ctx, "Room height:", 0, &new_room_height, 100, 1, 1);

                if (nk_button_label(ctx, "Create new room")) {
                }

                if (nk_button_label(ctx, "Back")) {
                        popup_new_up = false;
                        popup_menu_up = true;
                }
        }
        nk_end(ctx);
}

static struct nk_image*
create_tiles(const char* filename, const uint32_t width, const uint32_t height)
{
        int tileset_width, tileset_height;
        struct nk_image tileset = hs_nk_image_load_size_info(filename, &tileset_width, &tileset_height);
        struct nk_image* tiles = (struct nk_image*)malloc(sizeof(struct nk_image) * width * height);

        const int tile_width = tileset_width / width;
        const int tile_height = tileset_height / height;

        uint32_t i = 0;
        for (uint32_t y = 0; y < height; y++) {
                const uint32_t ypos = y * tile_height;
                for (uint32_t x = 0; x < width; x++) {
                        const uint32_t xpos = x * tile_width;
                        tiles[i++] = nk_subimage_handle(tileset.handle, tileset_width, tileset_height,
                                                        nk_rect(xpos, ypos, tile_width, tile_height));
                }
        }

        return tiles;
}

static inline void side_bar()
{
        static uint32_t tileset_chosen = 0;
        const char* const default_name = "No tileset selected";
        static char* filename = NULL;
        static char* filename_short = (char*)default_name;

        static struct nk_image* tileset_images = NULL;
        static uint32_t selected_tile = 0;
        static uint32_t tileset_image_count = 0;
        static uint32_t tileset_popup = 0;

        if (nk_begin(ctx, "Side bar", nk_rect(0, 0, 200, gd.height), 0)) {
                nk_layout_row_dynamic(ctx, 10, 1);
                nk_label(ctx, filename_short, NK_TEXT_CENTERED);

                nk_layout_row_dynamic(ctx, 30, 1);

                if (nk_button_label(ctx, "Choose new tileset")) {
                        filename = (char*)sfd_open_dialog(&(sfd_Options){
                                        .title = "Choose a tileset",
                                        .filter_name = "PNG image",
                                        .filter = "*.png",
                                });
                        if (filename && filename[0] != '\0') {
                                tileset_popup = true;

                                char* last_slash_pos = filename;
                                for (char* f = filename; f[1] != '\0'; f++)
                                        if (f[0] == '/' || f[0] == '\\')
                                                last_slash_pos = f + 1;
                                filename_short = last_slash_pos;
                        } else {
                                filename_short = (char*)default_name;
                        }
                }

                if (tileset_popup &&
                    nk_popup_begin(ctx, NK_POPUP_STATIC, "Select tileset size",
                                   NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE,
                                   nk_rect(200, 30, 320, 210))) {
                        static int tileset_width = 5;
                        static int tileset_height = 3;
                        static int tile_width = 32;
                        static int tile_height = 32;

                        nk_layout_row_dynamic(ctx, 30, 1);
                        nk_property_int(ctx, "Tileset width (in tiles):", 1, &tileset_width, 20, 1, 0.4f);
                        nk_property_int(ctx, "Tileset height (in tiles):", 1, &tileset_height, 50, 1, 0.5f);
                        nk_property_int(ctx, "Tile width (in px):", 8, &tile_width, 64, 1, 0.5f);
                        nk_property_int(ctx, "Tile height (in px):", 8, &tile_height, 64, 1, 0.5f);

                        nk_layout_row_dynamic(ctx, 30, 2);
                        if (nk_button_label(ctx, "Confirm")) {
                                if (!tileset_images)
                                        free(tileset_images);
                                tileset_images = create_tiles(filename, tileset_width, tileset_height);
                                if (!tileset_images) {
                                        filename = NULL;
                                        filename_short = (char*)default_name;
                                } else {
                                        tileset_image_count = tileset_width * tileset_height;
                                        tileset_chosen = true;
                                }
                                tileset_popup = false;
                                nk_popup_close(ctx);

                        }
                        if (nk_button_label(ctx, "Quit")) {
                                filename = NULL;
                                filename_short = (char*)default_name;
                                tileset_popup = false;
                                nk_popup_close(ctx);
                        }

                        nk_popup_end(ctx);
                }

                if (tileset_chosen) {
                        nk_layout_row_dynamic(ctx, 20, 1);
                        nk_label(ctx, "Selected tile:", NK_TEXT_CENTERED);
                        nk_layout_row_dynamic(ctx, 190, 1);
                        nk_image(ctx, tileset_images[selected_tile]);
                        nk_layout_row_dynamic(ctx, 60, 3);
                        for (uint32_t i = 0; i < tileset_image_count; i++) {
                                if (nk_button_image(ctx, tileset_images[i]))
                                        selected_tile = i;
                        }
                }
        }

        nk_end(ctx);
}

static inline void loop()
{
        if (!popup_new_up)
                if (hs_get_key_toggle(gd, escape) == HS_KEY_PRESSED) popup_menu_up = !popup_menu_up;

        side_bar();
        if (popup_menu_up) {
                popup_start();
        } else {
                if (popup_new_up)
                        popup_new_room();
        }

        /* Draw */
        glfwGetWindowSize(gd.window, (int*)&gd.width, (int*)&gd.height);
        glViewport(0, 0, gd.width, gd.height);

        hs_clear(0.2, 0.2, 0.2, 1.0, 0);

        /* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
         * with blending, scissor, face culling, depth test and viewport and
         * defaults everything back into a default state.
         * Make sure to either a.) save and restore or b.) reset your own state after
         * rendering the UI. */
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}

int main()
{
        init();
        hs_loop(gd, nk_glfw3_new_frame(&glfw); loop());

        nk_glfw3_shutdown(&glfw);
        hs_exit();
        return 0;
}
