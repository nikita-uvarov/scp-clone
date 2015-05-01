#ifndef SGE_COLLADA_MESH_LOADER
#define SGE_COLLADA_MESH_LOADER

#include "Common.h"

#include <string>
#include <vector>
#include <memory>

namespace sge
{

// normal mesh architecture:
// vertices array
// several indices arrays for multiple materials
// texture are stored by texture manager

class TextureReference
{
public :
    GLuint loadedId;
    
    TextureReference(): loadedId(0) {}
};

class MeshMaterial
{
public :
    TextureReference diffuseTexture;
    TextureReference lightingTexture;
};

class Vertex
{
public :
    vec3 position;
    vec3 skinnedPosition;
    
    void slowRender();
};

class Polylist
{
public :
    MeshMaterial material;
    std::vector<int> indices;
    
    void slowRender(std::vector<Vertex>& vertices);
};

enum class FloatVectorValueType
{
    FLOAT,
    FLOAT_2,
    FLOAT_3,
    FLOAT_4,
    FLOAT_4x4,
};

class FloatVectorValue
{
public :
    FloatVectorValueType type;
    std::vector<ftype> subvalues;
    
    template<class T>
    T getValue() const;
    
    void setValue(const mat4& mat);
    void setValue(const vec4& vec);
    void setValue(const vec3& vec);
    void setValue(ftype value);
};

template<>
mat4 FloatVectorValue::getValue() const;

template<>
vec4 FloatVectorValue::getValue() const;

template<>
vec3 FloatVectorValue::getValue() const;

template<>
vec2 FloatVectorValue::getValue() const;

template<>
ftype FloatVectorValue::getValue() const;

enum class TransformationType
{
    MATRIX,
    ROTATE,
    TRANSLATE,
    SCALE,
    
    DEBUG_ROTATE
    
    // hypothetical: SKEW, LOOKAT
};

class Mesh;

class AnimationChannel
{
public :
    std::vector<ftype> times;
    std::vector<FloatVectorValue> values;
    
    int jointIndex;
    int transformIndex;
    int subvalueIndex; // or -1
    
    void applyValue(Mesh& to, ftype t);
};

// represents a transformation specified by a floating point values array
class Transform
{
public :
    TransformationType type;
    FloatVectorValue value;
    
    void applyTransform(mat4& to) const;
};

class TransformStack
{
public :
    std::vector<Transform> transforms;

    void applyTransforms(mat4& to) const;
};

class Mesh;

class SkeletonJoint
{
public :
    int parentIndex;
    std::vector<int> childrenIndices;
    //mat4 transformMatrix;
    TransformStack transformStack;
    mat4 inverseBindMatrix;
    mat4 transformationMatrix;
    
    void slowRender(Mesh& mesh, mat4 parentTransform, bool renderSkeleton);
};

class Mesh
{
public :
    std::vector<Vertex> vertices;
    std::vector<std::vector<std::pair<int, ftype>>> vertexWeights;
    
    std::vector<Polylist> polylists;
    
    mat4 bindShapeMatrix;
    
    TransformStack armatureTransformStack;
    // the first is root joint
    std::vector<SkeletonJoint> joints;
    
    std::vector<AnimationChannel> animationChannels;
    
    bool renderSkeleton = true;
    
    void slowRender();
    void slowRenderPass(bool skeletonOnly);
    
    bool skinned = false;
    
    void applyAnimation();
    void applySkinning();
};

Mesh loadColladaMeshNew(std::string fileName);

}

#endif // SGE_CzOLLADA_MESH_LOADER
