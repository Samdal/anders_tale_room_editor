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

uint32_t tileset_tex;
hs_tilemap tilemap;
uint32_t selected_tile = 0;

sfd_Options anders_tale_room = {
  .title        = "Johan, you may now choose a Anders Tale Room file",
  .filter_name  = "Anders tale room",
  .filter       = "*.aroom",
};

uint32_t tilemap_offset_x, tilemap_offset_y, tilemap_screen_width, tilemap_screen_height;

static void
framebuffer_size_callback(GLFWwindow* win, const int width, const int height)
{
        gd.width = width;
        gd.height = height;
        const float viewport_height = (float)gd.width * ((float)tilemap.height/tilemap.width);
        if (viewport_height > gd.height) {
                const float viewport_width = (float)gd.height * ((float)tilemap.width/tilemap.height);
                tilemap_offset_x = ((gd.width - viewport_width) / 2);
                tilemap_offset_y = 0;
                tilemap_screen_width = viewport_width;
                tilemap_screen_height = gd.height;
        } else {
                tilemap_offset_x = 0;
                tilemap_offset_y = ((gd.height - viewport_height) / 2);
                tilemap_screen_width = gd.width;
                tilemap_screen_height = viewport_height;
        }

        tilemap_offset_x += 200;
        tilemap_screen_width -= 200;
}

static inline void init()
{
        hs_init(&gd, "Anders tale room editor (mfw Johan anderstaler istendefor å game)", framebuffer_size_callback, 0);

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
        if (nk_begin(ctx, "Create new room", nk_rect(gd.width/2 - 100, gd.height/2 - 70, 200, 140),
                    NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {
                static int room_width = 20;
                static int room_height = 20;

                nk_layout_row_dynamic(ctx, 30, 1);
                nk_property_int(ctx, "Room width:", 0, &room_width, 60, 1, 1);
                nk_property_int(ctx, "Room height:", 0, &room_height, 60, 1, 1);

                nk_layout_row_dynamic(ctx, 30, 2);
                if (nk_button_label(ctx, "Create room")) {
                        tilemap.width = room_width;
                        tilemap.height = room_height;
                        tilemap.tile_width = 1.0f/room_width;
                        tilemap.tile_height = 1.0f/room_height;

                        if (tileset_tex) {
                               hs_tilemap_init(&tilemap, tileset_tex, 0);
                        }

                        popup_new_up = false;
                        popup_menu_up = false;
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
        tileset_tex = hs_tex2d_create_size_info_pixel(filename, GL_RGBA, &tileset_width, &tileset_height);
        struct nk_image tileset = nk_image_id(tileset_tex);
        struct nk_image* tiles = (struct nk_image*)malloc(sizeof(struct nk_image) * width * height);
        assert(tiles);

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
                                   nk_rect(200, 30, 320, 140))) {
                        static int tileset_width = 5;
                        static int tileset_height = 3;

                        nk_layout_row_dynamic(ctx, 30, 1);
                        nk_property_int(ctx, "Tileset width (in tiles):", 1, &tileset_width, 20, 1, 0.4f);
                        nk_property_int(ctx, "Tileset height (in tiles):", 1, &tileset_height, 50, 1, 0.5f);

                        nk_layout_row_dynamic(ctx, 30, 2);
                        if (nk_button_label(ctx, "Confirm")) {
                                if (tileset_images) {
                                        free(tileset_images);
                                        glDeleteTextures(1, &tileset_tex);
                                        tileset_tex = 0;
                                }

                                tileset_images = create_tiles(filename, tileset_width, tileset_height);
                                tileset_image_count = tileset_width * tileset_height;

                                tilemap.tileset_width = tileset_width;
                                tilemap.tileset_height = tileset_height;
                                tilemap.sp.tex.tex_unit = tileset_tex;

                                tileset_chosen = true;
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
        } else if (popup_new_up) {
                popup_new_room();
        }

        glViewport(0, 0, gd.height, gd.width);
        hs_clear(0.2, 0.2, 0.2, 1.0, 0);
        glEnable(GL_BLEND);

        if (tilemap.vertices && gd.width > 200) {
                glViewport(tilemap_offset_x, tilemap_offset_y, tilemap_screen_width, tilemap_screen_height);

                hs_tex2d_activate(tilemap.sp.tex.tex_unit, GL_TEXTURE0);
                hs_tilemap_draw(tilemap);
                if (glfwGetMouseButton(gd.window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
                        double xpos, ypos;
                        glfwGetCursorPos(gd.window, &xpos, &ypos);
                        // reverse y axis
                        ypos -= gd.height;
                        ypos *= -1;

                        if(xpos > tilemap_offset_x && xpos < tilemap_offset_x + tilemap_screen_width &&
                           ypos > tilemap_offset_y && ypos < tilemap_offset_y + tilemap_screen_height) {
                                int32_t tilex = (xpos - tilemap_offset_x) / (tilemap.tile_width * tilemap_screen_width);
                                int32_t tiley = (ypos - tilemap_offset_y) / (tilemap.tile_height * tilemap_screen_height);
                                hs_tilemap_set_xy(&tilemap, tilex, tiley, selected_tile);
                                hs_tilemap_update_vbo(tilemap);
                        }
                }
        }

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
