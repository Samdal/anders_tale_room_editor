#define HS_IMPL
#define HS_NUKLEAR
#include "hs/hs_graphics.h"

hs_game_data gd = {
        .width = 960,
        .height = 540
};

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct nk_context *ctx;
struct nk_colorf bg;
struct nk_glfw glfw = {0};

static inline void
init()
{
        hs_init(&gd, "Anders tale room editor", NULL, 0);
        bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

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
loop()
{
        if (nk_begin(ctx, "Test", nk_rect(0, 0, 230, 250), 0))
        {
                enum {EASY, HARD};
                static int op = EASY;

                nk_layout_row_static(ctx, 30, 80, 1);
                if (nk_button_label(ctx, "button"))
                        fprintf(stdout, "button pressed\n");

                nk_layout_row_dynamic(ctx, 30, 2);
                if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
                if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, "background:", NK_TEXT_LEFT);
                nk_layout_row_dynamic(ctx, 25, 1);
                if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
                        nk_layout_row_dynamic(ctx, 120, 1);
                        bg = nk_color_picker(ctx, bg, NK_RGBA);
                        nk_layout_row_dynamic(ctx, 25, 1);
                        bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
                        bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
                        bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
                        bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
                        nk_combo_end(ctx);
                }
        } nk_end(ctx);

        /* Draw */
        glfwGetWindowSize(gd.window, (int*)&gd.width, (int*)&gd.height);
        glViewport(0, 0, gd.width, gd.height);

        hs_clear(bg.r, bg.g, bg.b, bg.a, 0);

        /* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
         * with blending, scissor, face culling, depth test and viewport and
         * defaults everything back into a default state.
         * Make sure to either a.) save and restore or b.) reset your own state after
         * rendering the UI. */
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}

int
main()
{
        init();
        hs_loop(gd, nk_glfw3_new_frame(&glfw); loop());

        nk_glfw3_shutdown(&glfw);
        glfwTerminate();
        return 0;
}
