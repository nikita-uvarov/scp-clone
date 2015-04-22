#ifndef SGE_COMMON_H
#define SGE_COMMON_H

#ifndef GL_GLEXT_PROTOTYPES
#error GL_GLEXT_PROTOTYPES should be defined
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include <type_traits>

namespace sge
{
    typedef glm::dmat2::value_type ftype;
    static_assert(std::is_same<double, ftype>::value,
                  "glm double precision type is different from 'double'");
    
    // a lot more than double's relative precision
    const ftype FTYPE_WEAK_EPS = 1e-8;
    
    bool weakEq(ftype a, ftype b);
    
    typedef glm::dmat2 mat2;
    typedef glm::dmat3 mat3;
    typedef glm::dmat4 mat4;
    
    typedef glm::dvec2 vec2;
    typedef glm::dvec3 vec3;
    typedef glm::dvec4 vec4;
}

#undef assert
#define assert(condition, ...) SDL_assert(condition)

#define critical_error(...) \
    { \
        fflush(stdout); \
        fflush(stderr); \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__); \
        fflush(stdout); \
        fflush(stderr); \
        SDL_Quit(); \
        exit(1); \
    }

#define verify(condition, ...) \
    if (!(condition)) \
        critical_error(__VA_ARGS__)

#define unreachable() critical_error("Unreachable line %s:%d reached.", __FILE__, __LINE__)

#endif // SGE_COMMON_H