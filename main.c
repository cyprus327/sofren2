#define SFT_IMPL
#define SFT_USE_CGLTF
#define CGLTF_IMPLEMENTATION
#define SFT_USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "sofren2.c"

#define SDL_MAIN_HANDLED
#include "../../../../../msys64/mingw64/include/SDL2/SDL.h"
// #include <SDL2/SDL.h>

#include <stdio.h>
#include <sys/stat.h>

#define WIDTH 1600
#define HEIGHT 900

struct bitmap_font {
    SDL_Texture* tex;
    i32 char_w, char_h, scale;
};

static u8 is_key_pressed(SDL_Scancode key) {
    static const u8 *keys = NULL;
    static u8 previous[SDL_NUM_SCANCODES] = {0};

    keys = SDL_GetKeyboardState(NULL);

    const u8 pressed = keys[key] && !previous[key];
    previous[key] = keys[key];

    return pressed;
}

static SDL_Texture* load_font_texture(SDL_Renderer* renderer, const char* path) {
    const sft_tex_handle sfttex = sft_texture_load(path);
    if (SFT_INVALID_HANDLE == sfttex) {
        SFT_ERR_EXIT("failed to load texture, exiting\n");
    }

    const i32 w = sft_texture_get_width(sfttex);
    const i32 h = sft_texture_get_height(sfttex);

    SDL_Texture* sdltex = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        w, h);
    if (!sdltex) {
        SFT_ERR_EXIT("failed to create SDL_Texture\n");
    }

    u32* linear_pixels = sft_texture_get_linear_pixels(sfttex);
    
    SDL_UpdateTexture(sdltex, NULL, linear_pixels, w * sizeof(u32));
    SDL_SetTextureBlendMode(sdltex, SDL_BLENDMODE_BLEND);

    free(linear_pixels);
    sft_texture_release(sfttex);

    return sdltex;
}

static void draw_text(SDL_Renderer* renderer, const struct bitmap_font* font, const char* text, i32 x, i32 y) {
    i32 curr_x = x, curr_y = y;

    const i32 cols = 16;

    for (i32 i = 0; '\0' != text[i]; i += 1) {
        const u8 c = (u8)text[i];

        if ('\n' == c) {
            curr_x = x;
            curr_y += (font->char_h * font->scale);
            continue;
        }

        SDL_Rect src_rect;
        src_rect.x = (c % cols) * font->char_w;
        src_rect.y = (c / cols) * font->char_h;
        src_rect.w = font->char_w, src_rect.h = font->char_h;

        SDL_Rect dest_rect;
        dest_rect.x = curr_x;
        dest_rect.y = curr_y;
        dest_rect.w = font->char_w * font->scale;
        dest_rect.h = font->char_h * font->scale;

        SDL_RenderCopy(renderer, font->tex, &src_rect, &dest_rect);

        curr_x += font->char_w * font->scale;
    }
}

static void draw_ui(SDL_Renderer* renderer, const struct bitmap_font* font, f32 frame_time) {
    static f32 fps_timer = 0.f;
    static i32 fps_frames = 0;
    static f32 avg_fps = 0.f;

    fps_timer += frame_time;
    fps_frames += 1;

    const f32 fps_refresh = 0.33334f;
    if (fps_timer >= fps_refresh) {
        avg_fps = (f32)fps_frames / fps_timer;
        fps_timer -= fps_refresh;
        fps_frames = 0;
    }

    char fps_buf[32];
    snprintf(fps_buf, sizeof(fps_buf), "FPS: %.1f", avg_fps);
    draw_text(renderer, font, fps_buf, 16, 16);

    // { // update debug mode
    //     static i32 mode = 0;
    //     if (is_key_pressed(SDL_SCANCODE_Z)) {
    //         mode = (mode + 1) % SFT_DEBUG_COUNT;
    //     }
    //     sft_set_debug_mode(mode);
    //     if (SFT_DEBUG_NONE != mode) {
    //         char buf[64];
    //         snprintf(buf, sizeof(buf), "Debug Mode: %s", sft_get_debug_name(mode));
    //         draw_text(renderer, font, buf, 16, 32);
    //     }
    // }
}

static void save_camera_state(const char* model_path, sftvec cam_pos, f32 yaw, f32 pitch) {
    char path[1024];
    snprintf(path, sizeof(path), "%s.cam", model_path);
    FILE* f = fopen(path, "wb");
    if (f) {
        fwrite(&cam_pos, sizeof(sftvec), 1, f);
        fwrite(&yaw, sizeof(f32), 1, f);
        fwrite(&pitch, sizeof(f32), 1, f);
        fclose(f);
        printf("saved camera state to %s\n", path);
    }
    else {
        printf("failed to save camera state to %s\n", path);
    }
}

static i32 load_camera_state(const char* model_path, sftvec* cam_pos, f32* yaw, f32* pitch) {
    char path[1024];
    snprintf(path, sizeof(path), "%s.cam", model_path);
    FILE* f = fopen(path, "rb");
    if (f) {
        const u64 r1 = fread(cam_pos, sizeof(sftvec), 1, f);
        const u64 r2 = fread(yaw, sizeof(f32), 1, f);
        const u64 r3 = fread(pitch, sizeof(f32), 1, f);
        fclose(f);
        if (1 == r1 && 1 == r2 && 1 == r3) {
            printf("loaded camera state from %s\n", path);
            return 1;
        }
    }
    return 0;
}

// returns 0 on successful update
static u8 map_check_and_update(const char* path, sft_arena* map_arena, sft_instance* map) {
    if (!map_arena) {
        SFT_ERR_RET(1, "map_arena is NULL\n");
    }
    if (!map) {
        SFT_ERR_RET(1, "map instance is NULL\n");
    }

    // get current file stats
    struct stat file_stat;
    const i32 stat_res = stat(path, &file_stat);
    if (0 != stat_res) {
        SFT_ERR_RET(1, "stat failed with %d\n", stat_res);
    }

    static time_t last_modified_time = 0;
    if (file_stat.st_mtime <= last_modified_time) {
        // already up to date
        return 0;
    }
    last_modified_time = file_stat.st_mtime;

    // load new model
    const sft_model* model = sft_model_load_glb(path, map_arena, 0);
    if (!model) {
        SFT_ERR_RET(1, "failed to load new map from '%s'\n", path);
    }

    // clear arena to "free" old model
    sft_arena_frame_begin(map_arena);
    map->model = model;

    return 0;
}

i32 main(i32 argc, char** argv) {
    if (2 != argc) {
        printf("usage: %s <map.glb>\n", argv[0]);
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_Window* window = SDL_CreateWindow("Window", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    sft_init(WIDTH, HEIGHT, 0);

    sft_arena map_arena;
    sft_arena_init(&map_arena, 10ULL * 1024 * 1024 * 1024);

    struct bitmap_font font = {0};
    font.tex = load_font_texture(renderer, "../stealth/assets/Fnord_16x16.png");
    font.char_w = 16;
    font.char_h = 16;
    font.scale  = 1;

    sft_instance map_inst = {0};
    const char* map_path = argv[1];
    { // load map
        printf("loading %s...\n", map_path);
        if (0 != map_check_and_update(map_path, &map_arena, &map_inst)) {
            return 1;
        }
        map_inst.transform = sft_m_identity();
    }
    
    SDL_SetRelativeMouseMode(SDL_TRUE);

    u64 prev_time = SDL_GetPerformanceCounter();
    const f32 freq = (f32)SDL_GetPerformanceFrequency();
    
    f32 yaw = 0.f;
    f32 pitch = 0.f;
    sftvec cam_pos = SFT_V(0.f, 0.f, -5.f);
    
    load_camera_state(map_path, &cam_pos, &yaw, &pitch);

    i32 running = 1;
    while (running) {
        const u64 curr_time = SDL_GetPerformanceCounter();
        const f32 dt = (curr_time - prev_time) / freq;
        prev_time = curr_time;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: {
                    running = 0;
                } break;

                case SDL_KEYDOWN: {
                    switch (e.key.keysym.sym) {
                        case SDLK_ESCAPE: {
                            running = 0;
                        } break;
                        case SDLK_n: {
                            save_camera_state(map_path, cam_pos, yaw, pitch);
                        } break;
                        case SDLK_m: {
                            load_camera_state(map_path, &cam_pos, &yaw, &pitch);
                        } break;
                        default: break;
                    }
                } break;

                case SDL_MOUSEMOTION: {
                    yaw += e.motion.xrel * 0.002f;
                    pitch -= e.motion.yrel * 0.002f;
                    
                    const f32 pitch_limit = SFT_PI / 2.f - 0.05f;
                    if (pitch > pitch_limit) pitch = pitch_limit;
                    if (pitch < -pitch_limit) pitch = -pitch_limit;
                } break;

                default: break;
            }
        }

        const sftvec forward = SFT_V(
            cosf(pitch) * sinf(yaw),
            sinf(pitch),
            cosf(pitch) * cosf(yaw));
        const sftvec world_up = SFT_V(0.f, 1.f, 0.f);
        const sftvec right = sft_v_norm(sft_v_cross(world_up, forward));

        const u8* keys = SDL_GetKeyboardState(NULL);
        f32 speed = (keys[SDL_SCANCODE_LSHIFT] ? 10.f : 3.f) * dt;
        
        if (keys[SDL_SCANCODE_W]) cam_pos = sft_v_add(cam_pos, sft_v_mul(forward, speed));
        if (keys[SDL_SCANCODE_S]) cam_pos = sft_v_sub(cam_pos, sft_v_mul(forward, speed));
        if (keys[SDL_SCANCODE_A]) cam_pos = sft_v_sub(cam_pos, sft_v_mul(right, speed));
        if (keys[SDL_SCANCODE_D]) cam_pos = sft_v_add(cam_pos, sft_v_mul(right, speed));
        if (keys[SDL_SCANCODE_E] || keys[SDL_SCANCODE_SPACE]) cam_pos = sft_v_add(cam_pos, sft_v_mul(world_up, speed));
        if (keys[SDL_SCANCODE_Q] || keys[SDL_SCANCODE_LCTRL]) cam_pos = sft_v_sub(cam_pos, sft_v_mul(world_up, speed));

        { // update lighting state
            // doesn't actually have to be per frame
            sft_set_ambient_light(0.125f, 0.125f, 0.125f);

            // does have to be per frame
            // sft_draw_directional_light(sft_v_normf(0.2f, 0.7f, 0.5f), 1.f, 1.f, 1.f, 0.8f);

            static sftvec lp = {0, 7, 0};
            if (keys[SDL_SCANCODE_X]) {
                lp = cam_pos;
            }
            sft_draw_point_light(lp, 1.f, 1.f, 1.f, 1.5f, 30.f);
            // sft_draw_point_light(SFT_V(sinf(curr_time / freq) * 5.f, 5.f, 0.f), 0.f, 0.7f, 1.f, 2.f, 10.f);
            sft_draw_point_light(SFT_V(0.f, 7.f, 0.f), 0.f, 0.7f, 1.f, 2.f, 8.f);
        }

        { // begin frame
            void* locked_pixels;
            i32 pitch;
            SDL_LockTexture(texture, NULL, &locked_pixels, &pitch);
            sft_state->display.color_buffer = (u32*)locked_pixels;
            sft_frame_begin();
        }

        { // draw frame
            const sftvec cam_target = sft_v_add(cam_pos, forward);
            const sftmat view = sft_m_view_look_at(cam_pos, cam_target, world_up);
            const sftmat proj = sft_m_proj(70.f, (f32)WIDTH / (f32)HEIGHT, 0.1f, 1000.f);
            sft_set_camera(cam_pos, view, proj);
    
            sft_draw_instance(&map_inst);

            sft_present(); 
        }

        { // end frame
            sft_frame_end();
            SDL_UnlockTexture(texture);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            draw_ui(renderer, &font, dt);
            SDL_RenderPresent(renderer);

            memset(sft_state->display.color_buffer, 0x11, WIDTH * HEIGHT * sizeof(u32));
        }
    }

    sft_release();

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
