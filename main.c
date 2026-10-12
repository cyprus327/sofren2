#define SFT_IMPL
#define SFT_USE_CGLTF
#define SFT_CGLTF_PATH "optional/cgltf.h"
#define CGLTF_IMPLEMENTATION
#define SFT_USE_STB_IMAGE
#define SFT_STB_IMAGE_PATH "optional/stb_image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "sofren2.c"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>

#define MAX_MESSAGES 8
#define MESSAGE_LIFETIME 3.f

struct bitmap_font {
    SDL_Texture* tex;
    i32 char_w, char_h, scale;
};

struct ui_message {
    char text[128];
    f32 timer;
};


struct qsf_light {
    u32 type;
    f32 pos[3];
    f32 color[3];
    f32 intensity;
    f32 attenuation;
};

static struct qsf_light* game_lights = NULL;
static i32 game_light_count = 0;

static struct ui_message ui_messages[MAX_MESSAGES];

static void push_message(const char* fmt, ...) {
    i32 slot = -1;
    for (i32 i = 0; i < MAX_MESSAGES; i += 1) {
        if (ui_messages[i].timer <= 0.f) {
            slot = i;
            break;
        }
    }
    
    if (-1 == slot) {
        for (i32 i = 0; i < MAX_MESSAGES - 1; i += 1) {
            ui_messages[i] = ui_messages[i + 1];
        }
        slot = MAX_MESSAGES - 1;
    }
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(ui_messages[slot].text, sizeof(ui_messages[slot].text), fmt, args);
    va_end(args);
    ui_messages[slot].timer = MESSAGE_LIFETIME;
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

static void update_and_draw_messages(SDL_Renderer* renderer, const struct bitmap_font* font, f32 dt, i32 window_width) {
    i32 msg_y = 16;
    
    for (i32 i = 0; i < MAX_MESSAGES; i += 1) {
        if (ui_messages[i].timer > 0.f) {
            ui_messages[i].timer -= dt;
            if (ui_messages[i].timer <= 0.f) {
                ui_messages[i].text[0] = '\0';
            }
        }
    }
    
    for (i32 i = 0; i < MAX_MESSAGES - 1; i += 1) {
        if (ui_messages[i].timer <= 0.f) {
            for (i32 j = i + 1; j < MAX_MESSAGES; j += 1) {
                if (ui_messages[j].timer > 0.f) {
                    ui_messages[i] = ui_messages[j];
                    ui_messages[j].timer = 0.f;
                    break;
                }
            }
        }
    }
    
    for (i32 i = 0; i < MAX_MESSAGES; i += 1) {
        if (ui_messages[i].timer > 0.f) {
            const i32 len = strlen(ui_messages[i].text);
            const i32 w = len * font->char_w * font->scale;
            const i32 x = (window_width - w) / 2;
            
            draw_text(renderer, font, ui_messages[i].text, x, msg_y);
            msg_y += font->char_h * font->scale + 4;
        }
    }
}

static void draw_ui(SDL_Renderer* renderer, const struct bitmap_font* font, f32 frame_time, i32 window_width) {
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

    update_and_draw_messages(renderer, font, frame_time, window_width);
}

static u8 is_glb(const char* path) {
    const char* ext = strrchr(path, '.');
    return (ext && strcmp(ext, ".glb") == 0);
}

static u8 map_check_and_update(const char* path, sft_arena* map_arena, sft_instance* map) {
    if (!map_arena || !map) {
        return 1;
    }

    struct stat file_stat;
    if (0 != stat(path, &file_stat)) {
        return 1;
    }

    static time_t last_modified_time = 0;
    if (file_stat.st_mtime <= last_modified_time) {
        return 0;
    }
    last_modified_time = file_stat.st_mtime;

    const u8 is_glb_file = is_glb(path);
    const sft_model* model = is_glb_file ? sft_model_load_glb(path, map_arena, 0, 0) : sft_model_load_qsf(path, map_arena);

    if (!model) {
        SFT_ERR_RET(1, "failed to load new map from '%s'\n", path);
    }

    if (!is_glb_file) {
        FILE* f = fopen(path, "rb");
        if (f) {
            fseek(f, 8, SEEK_SET);
            u64 game_data_offset = 0;
            fread(&game_data_offset, 8, 1, f);
            fseek(f, game_data_offset, SEEK_SET);
            
            if (game_lights) {
                free(game_lights);
            }
            game_lights = NULL;

            fread(&game_light_count, 4, 1, f);
            if (game_light_count > 0) {
                game_lights = malloc(game_light_count * sizeof(struct qsf_light));
                fread(game_lights, sizeof(struct qsf_light), game_light_count, f);
                for (i32 i = 0; i < game_light_count; i += 1) {
                    game_lights[i].attenuation *= 2.f;
                }
            }
            fclose(f);
        }
    }
    else {
        if (game_lights) {
            free(game_lights);
        }
        game_lights = NULL;
        game_light_count = 0;
    }

    sft_arena_frame_begin(map_arena);
    map->model = model;
    return 0;
}

i32 main(i32 argc, char** argv) {
    if (2 != argc) {
        printf("usage: %s <map.glb or map.qsf>\n", argv[0]);
        return 1;
    }

    i32 window_width = 1600;
    i32 window_height = 900;
    f32 render_scale = 1.f;
    i32 render_w = (i32)(window_width * render_scale);
    i32 render_h = (i32)(window_height * render_scale);

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_Window* window = SDL_CreateWindow("Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, render_w, render_h);

    sft_init(render_w, render_h, 0);
    sft_set_clear(1);
    sft_set_shadows_enabled(1);
    sft_set_lightmap_brightness(1.5f);

    sft_arena map_arena;
    sft_arena_init(&map_arena, 10ULL * 1024 * 1024 * 1024);

    struct bitmap_font font = {0};
    font.tex = load_font_texture(renderer, "res/fnord1616.png");
    font.char_w = 16;
    font.char_h = 16;
    font.scale  = 1;

    sft_instance map_inst = {0};
    map_check_and_update(argv[1], &map_arena, &map_inst);
    map_inst.transform = sft_m_identity();

    SDL_SetRelativeMouseMode(SDL_TRUE);

    u64 prev_time = SDL_GetPerformanceCounter();
    const f32 freq = (f32)SDL_GetPerformanceFrequency();
    
    sftvec cam_pos = SFT_V(0.f, 2.f, 0.f);
    f32 cam_yaw = 0.f, cam_pitch = 0.f;

    u8 custom_light_active = 0;
    sftvec custom_light_pos = {0};
    
    u8 running = 1;
    while (running) {
        const u64 curr_time = SDL_GetPerformanceCounter();
        const f32 dt = (curr_time - prev_time) / freq;
        prev_time = curr_time;

        u8 update_render_target = 0;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (SDL_QUIT == e.type) {
                running = 0;
            }
            else if (SDL_KEYDOWN == e.type) {
                if (SDLK_ESCAPE == e.key.keysym.sym) {
                    running = 0;
                }
                else if (SDLK_RIGHTBRACKET == e.key.keysym.sym) {
                    render_scale += 0.1f;
                    if (render_scale > 2.f) {
                        render_scale = 2.f;
                    }
                    update_render_target = 1;
                    push_message("render scale: %.1fx", render_scale);
                }
                else if (SDLK_LEFTBRACKET == e.key.keysym.sym) {
                    render_scale -= 0.1f;
                    if (render_scale < 0.1f) {
                        render_scale = 0.1f;
                    }
                    update_render_target = 1;
                    push_message("render scale: %.1fx", render_scale);
                }
                else if (SDLK_x == e.key.keysym.sym) {
                    custom_light_active = 1;
                    custom_light_pos = cam_pos;
                    push_message("light relocated");
                }
            } 
            else if (SDL_WINDOWEVENT == e.type) {
                if (SDL_WINDOWEVENT_RESIZED == e.window.event) {
                    window_width = e.window.data1;
                    window_height = e.window.data2;
                    update_render_target = 1;
                }
            }
            else if (SDL_MOUSEMOTION == e.type) {
                cam_yaw += e.motion.xrel * 0.002f;
                cam_pitch -= e.motion.yrel * 0.002f;
                if (cam_pitch > 1.5f) cam_pitch = 1.5f;
                if (cam_pitch < -1.5f) cam_pitch = -1.5f;
            }
        }

        if (update_render_target) {
            render_w = (i32)(window_width * render_scale);
            if (render_w < 1) render_w = 1;
            render_h = (i32)(window_height * render_scale);
            if (render_h < 1) render_h = 1;
            
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, render_w, render_h);
            sft_resize(render_w, render_h);
        }

        const u8* keys = SDL_GetKeyboardState(NULL);

        if (keys[SDL_SCANCODE_C]) {
            sft_set_debug_mode(SFT_DEBUG_CHECKERBOARD);
        }
        else {
            sft_set_debug_mode(SFT_DEBUG_NONE);
        }

        const sftvec cam_fwd = SFT_V(cosf(cam_pitch) * sinf(cam_yaw), sinf(cam_pitch), cosf(cam_pitch) * cosf(cam_yaw));
        const sftvec cam_rgt = sft_v_norm(SFT_V(cosf(cam_yaw), 0.f, -sinf(cam_yaw)));
        const sftvec cam_up  = SFT_V(0.f, 1.f, 0.f);

        sftvec move_input = SFT_V(0, 0, 0);
        if (keys[SDL_SCANCODE_W]) move_input.z += 1.f;
        if (keys[SDL_SCANCODE_S]) move_input.z -= 1.f;
        if (keys[SDL_SCANCODE_A]) move_input.x -= 1.f;
        if (keys[SDL_SCANCODE_D]) move_input.x += 1.f;
        if (keys[SDL_SCANCODE_SPACE]) move_input.y += 1.f;
        if (keys[SDL_SCANCODE_LCTRL]) move_input.y -= 1.f;

        sftvec velocity = SFT_V(0, 0, 0);
        velocity = sft_v_add(velocity, sft_v_mul(cam_fwd, move_input.z));
        velocity = sft_v_add(velocity, sft_v_mul(cam_rgt, move_input.x));
        velocity = sft_v_add(velocity, sft_v_mul(cam_up,  move_input.y));

        if (sft_v_length2(velocity) > 0.01f) {
            velocity = sft_v_norm(velocity);
            f32 move_speed = keys[SDL_SCANCODE_LSHIFT] ? 15.f : 5.f;
            cam_pos = sft_v_add(cam_pos, sft_v_mul(velocity, move_speed * dt));
        }

        const sftvec cam_target = sft_v_add(cam_pos, cam_fwd);

        sft_set_ambient_light(0.35, 0.35, 0.35);
        if (game_light_count) {
            for (int i = 0; i < game_light_count; i += 1) {
                sft_draw_point_light(SFT_V(game_lights[i].pos[0], game_lights[i].pos[1], game_lights[i].pos[2]),
                    game_lights[i].color[0], game_lights[i].color[1], game_lights[i].color[2], 1.f, game_lights[i].attenuation / 2.f, i + 1);
            }
        }
        else {
            sft_draw_directional_light(sft_v_norm(SFT_V(1, 0.5, 0)), 1.f, 1.f, 1.f, 0.8f);
        }

        if (custom_light_active) {
            sft_draw_point_light(custom_light_pos, 1.f, 1.f, 0.f, 1.5f, 10.f, game_light_count + 1);
        }

        void* locked_pixels;
        i32 pitch_bytes;
        SDL_LockTexture(texture, NULL, &locked_pixels, &pitch_bytes);
        sft_set_color_buffer((u32*)locked_pixels, pitch_bytes);
        sft_frame_begin();

        sft_set_camera(cam_pos, sft_m_view_look_at(cam_pos, cam_target, SFT_V(0.f, 1.f, 0.f)), 
                       sft_m_proj(70.f, (f32)render_w / (f32)render_h, 0.1f, 1000.f));
        
        if (map_inst.model) {
            sft_draw_instance(&map_inst);
        }

        sft_present(); 
        sft_frame_end();
        
        SDL_UnlockTexture(texture);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        draw_ui(renderer, &font, dt, window_width);

        SDL_RenderPresent(renderer);
    }

    sft_release();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}