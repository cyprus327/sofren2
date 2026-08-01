#define SFT_IMPL
#define SFT_USE_CGLTF
#define SFT_CGLTF_PATH "../sofren/optional/cgltf.h"
#include "sofren2.c"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WIDTH 1600
#define HEIGHT 900

i32 main(i32 argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <model1.glb> [model2.glb...]\n", argv[0]);
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_Window* window = SDL_CreateWindow("sofren2 - Model Viewer", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    sft_init(WIDTH, HEIGHT, 0);

    sft_arena persistent_arena;
    sft_arena_init(&persistent_arena, 10ULL * 1024 * 1024 * 1024);

    i32 num_models = argc - 1;
    sft_model** models = (sft_model**)malloc(sizeof(sft_model*) * num_models);
    
    for (i32 i = 0; i < num_models; i++) {
        printf("Loading %s...\n", argv[i + 1]);
        models[i] = sft_load_gltf(argv[i + 1], &persistent_arena, 0);
        if (!models[i]) {
            printf("Failed to load %s\n", argv[i + 1]);
        }
    }
    i32 current_model = 0;

    SDL_SetRelativeMouseMode(SDL_TRUE);

    Uint64 prev_time = SDL_GetPerformanceCounter();
    const f32 freq = (f32)SDL_GetPerformanceFrequency();
    
    f32 yaw = 0.f;
    f32 pitch = 0.f;
    sftvec cam_pos = SFT_V(0.f, 0.f, -5.f);
    
    i32 running = 1;
    while (running) {
        Uint64 curr_time = SDL_GetPerformanceCounter();
        f32 dt = (curr_time - prev_time) / freq;
        prev_time = curr_time;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                }
                if (e.key.keysym.sym == SDLK_TAB) {
                    current_model = (current_model + 1) % num_models;
                    printf("Switched to model: %s\n", argv[current_model + 1]);
                }
            }
            
            if (e.type == SDL_MOUSEMOTION) {
                yaw += e.motion.xrel * 0.002f;
                pitch -= e.motion.yrel * 0.002f;
                
                const f32 pitch_limit = SFT_PI / 2.f - 0.05f;
                if (pitch > pitch_limit) pitch = pitch_limit;
                if (pitch < -pitch_limit) pitch = -pitch_limit;
            }
        }

        sftvec forward = SFT_V(
            cosf(pitch) * sinf(yaw),
            sinf(pitch),
            cosf(pitch) * cosf(yaw));
        sftvec world_up = SFT_V(0.f, 1.f, 0.f);
        sftvec right = sft_v_norm(sft_v_cross(world_up, forward));

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        f32 speed = (keys[SDL_SCANCODE_LSHIFT] ? 10.f : 3.f) * dt;
        
        if (keys[SDL_SCANCODE_W]) cam_pos = sft_v_add(cam_pos, sft_v_mul(forward, speed));
        if (keys[SDL_SCANCODE_S]) cam_pos = sft_v_sub(cam_pos, sft_v_mul(forward, speed));
        if (keys[SDL_SCANCODE_A]) cam_pos = sft_v_sub(cam_pos, sft_v_mul(right, speed));
        if (keys[SDL_SCANCODE_D]) cam_pos = sft_v_add(cam_pos, sft_v_mul(right, speed));
        if (keys[SDL_SCANCODE_E] || keys[SDL_SCANCODE_SPACE]) cam_pos = sft_v_add(cam_pos, sft_v_mul(world_up, speed));
        if (keys[SDL_SCANCODE_Q] || keys[SDL_SCANCODE_LCTRL]) cam_pos = sft_v_sub(cam_pos, sft_v_mul(world_up, speed));

        sft_frame_begin();

        sftvec cam_target = sft_v_add(cam_pos, forward);
        sftmat view = sft_m_view_look_at(cam_pos, cam_target, world_up);
        sftmat proj = sft_m_proj(60.f, (f32)WIDTH / (f32)HEIGHT, 0.1f, 1000.f);
        sft_set_camera(cam_pos, view, proj);

        if (models[current_model]) {
            sft_instance inst = {0};
            inst.model = models[current_model];
            inst.transform = sft_m_identity();
            sft_draw_instance(&inst);
        }
        
        sft_present(); 
        sft_frame_end();

        SDL_UpdateTexture(texture, NULL, sft_state->display.color_buffer, WIDTH * sizeof(u32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        memset(sft_state->display.color_buffer, 0x11, WIDTH * HEIGHT * sizeof(u32));
    }

    free(models);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}