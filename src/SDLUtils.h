#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#define SDL_VerifyResult(condition, ...) \
    if (!(condition)) \
    { \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__); \
        SDL_Quit(); \
        exit(1); \
    }

#endif // SDL_UTILS_H