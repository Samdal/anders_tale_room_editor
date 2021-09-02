#define HS_IMPL
#define HS_NUKLEAR
#define HS_SFD
#include "hs/hs_graphics.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct nk_context *ctx;
struct nk_glfw glfw = {0};

hs_tilemap tilemap;
uint32_t selected_tile = 0;
hs_aroom current_room;

uint32_t popup_new_room_on = 0;

hs_game_data gd = {
        .width = 960,
        .height = 540
};

sfd_Options anders_tale_room = {
  .title        = "Johan, you may now choose an Anders Tale Room file",
  .filter_name  = "Anders tale room",
  .filter       = "*.aroom",
};

uint32_t tilemap_offset_x, tilemap_offset_y, tilemap_screen_width, tilemap_screen_height;

static void
framebuffer_size_callback(GLFWwindow* win, const int width, const int height)
{
        gd.width = width;
        gd.height = height;
        if (tilemap.vertices) {
                const float viewport_height = (float)gd.width * ((float)tilemap.height/tilemap.width);
                if (viewport_height > gd.height) {
                        const float viewport_width = (float)gd.height * ((float)tilemap.width/tilemap.height);
                        tilemap_offset_x = (gd.width - viewport_width) / 2;
                        tilemap_offset_y = 0;
                        tilemap_screen_width = viewport_width;
                        tilemap_screen_height = gd.height;
                } else {
                        tilemap_offset_x = 0;
                        tilemap_offset_y = (gd.height - viewport_height) / 2;
                        tilemap_screen_width = gd.width;
                        tilemap_screen_height = viewport_height;
                }

                tilemap_offset_x += 200;
                tilemap_screen_width -= 200;
        }
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

static inline void
init_new_room(const uint32_t room_width, const uint32_t room_height)
{
        hs_tilemap_init(&tilemap, tilemap.sp.tex.tex_unit, 0);
        framebuffer_size_callback(gd.window, gd.width, gd.height);

        if (current_room.data)
                free(current_room.data);
        uint16_t* room_data = malloc(sizeof(uint16_t) * room_width * room_height);
        assert(room_data);

        current_room = (hs_aroom){
                .width = room_width,
                .height = room_height,
                .data = room_data,
        };
}

static inline uint32_t
popup_create_new_room()
{
        uint32_t new_tilemap = false;
        if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Create new room",
                           NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE,
                           nk_rect(200, 10, 320, 140))) {
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

                        init_new_room(room_width, room_height);

                        popup_new_room_on = false;
                        new_tilemap = true;
                        nk_popup_close(ctx);
                }

                if (nk_button_label(ctx, "Quit")) {
                        popup_new_room_on = false;
                        nk_popup_close(ctx);
                }
                nk_popup_end(ctx);
        }
        return new_tilemap;
}

static struct nk_image*
create_tiles(const char* filename, const uint32_t width, const uint32_t height)
{
        int tileset_width, tileset_height;
        tilemap.sp.tex.tex_unit = hs_tex2d_create_size_info_pixel(filename, GL_RGBA, &tileset_width, &tileset_height);
        struct nk_image tileset = nk_image_id(tilemap.sp.tex.tex_unit);
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

static inline void
side_bar()
{
        const char* const default_name = "No tileset selected";
        static char* filename = NULL;
        static char* filename_short = (char*)default_name;

        const char* const not_selected = "No tilemap selected";
        const char* const edited = "Tilemap edited";
        const char* const saved = "All changes saved";
        static char* room_editing_state = (char*)not_selected;

        static struct nk_image* tileset_images = NULL;
        static uint32_t tileset_image_count = 0;
        static uint32_t tileset_popup_on = 0;

        if (nk_begin(ctx, "Side bar", nk_rect(0, 0, 200, gd.height), 0)) {
                nk_layout_row_dynamic(ctx, 10, 1);
                nk_label(ctx, room_editing_state, NK_TEXT_CENTERED);

                nk_layout_row_dynamic(ctx, 30, 1);
                if (nk_button_label(ctx, "New room")) {
                        popup_new_room_on = true;
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
                if (popup_new_room_on) {
                        if (popup_create_new_room()) {
                                room_editing_state = (char*)edited;
                        }
                }

                nk_layout_row_dynamic(ctx, 10, 1);
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
                                tileset_popup_on = true;

                                char* last_slash_pos = filename;
                                for (char* f = filename; f[1] != '\0'; f++)
                                        if (f[0] == '/' || f[0] == '\\')
                                                last_slash_pos = f + 1;
                                filename_short = last_slash_pos;
                        } else {
                                filename_short = (char*)default_name;
                        }
                }

                static uint32_t tileset_created = false;

                if (tileset_popup_on &&
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
                                        glDeleteTextures(1, &tilemap.sp.tex.tex_unit);
                                        tilemap.sp.tex.tex_unit = 0;
                                }

                                tileset_images = create_tiles(filename, tileset_width, tileset_height);
                                tileset_image_count = tileset_width * tileset_height;

                                tilemap.tileset_width = tileset_width;
                                tilemap.tileset_height = tileset_height;
                                tilemap.sp.tex.tex_unit = tilemap.sp.tex.tex_unit;

                                if (tilemap.width) {
                                        //TODO change texture and offsets instead of recreating tilemap
                                        hs_tilemap_init(&tilemap, tilemap.sp.tex.tex_unit, 0);
                                        framebuffer_size_callback(gd.window, gd.width, gd.height);
                                }

                                tileset_popup_on = false;
                                tileset_created = true;
                                nk_popup_close(ctx);
                        }
                        if (nk_button_label(ctx, "Quit")) {
                                filename = NULL;
                                filename_short = (char*)default_name;
                                tileset_popup_on = false;
                                nk_popup_close(ctx);
                        }

                        nk_popup_end(ctx);
                }

                if (tileset_created) {
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
        glViewport(0, 0, gd.height, gd.width);
        hs_clear(0.2, 0.2, 0.2, 1.0, 0);
        glEnable(GL_BLEND);

        if (tilemap.vertices && gd.width > 200) {
                glViewport(tilemap_offset_x, tilemap_offset_y, tilemap_screen_width, tilemap_screen_height);

                hs_tex2d_activate(tilemap.sp.tex.tex_unit, GL_TEXTURE0);
                hs_tilemap_draw(tilemap);
                if (tilemap.sp.tex.tex_unit && glfwGetMouseButton(gd.window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
                        double xpos, ypos;
                        glfwGetCursorPos(gd.window, &xpos, &ypos);

                        // reverse y axis
                        ypos -= gd.height;
                        ypos *= -1;
                        int32_t tilex = (xpos - tilemap_offset_x) / (tilemap.tile_width * tilemap_screen_width);
                        int32_t tiley = (ypos - tilemap_offset_y) / (tilemap.tile_height * tilemap_screen_height);
                        if (hs_aroom_get_xy(current_room, tilex, tiley) != selected_tile &&
                            xpos > tilemap_offset_x && xpos < tilemap_offset_x + tilemap_screen_width &&
                            ypos > tilemap_offset_y && ypos < tilemap_offset_y + tilemap_screen_height) {
                                puts("edited");
                                hs_tilemap_set_xy(&tilemap, tilex, tiley, selected_tile);
                                hs_aroom_set_xy(&current_room, tilex, tiley, selected_tile);
                                hs_tilemap_update_vbo(tilemap);
                        }
                }
        }

        side_bar();

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
