#include "ColladaMeshLoader.h"

#include <pugixml.hpp>

#include <map>
#include <memory>
#include <vector>
#include <sstream>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <queue>

using namespace std;
using namespace sge;
using namespace pugi;

class ColladaMeshLoader;

// a document is parsed into two passes:

class Element
{
public :
    string id, sid;
    xml_node createdFrom;
    
    virtual ~Element() {}
    
    virtual void parseFromNode(ColladaMeshLoader& /*loader*/) {}
    virtual void resolveLinks(ColladaMeshLoader& /*loader*/) {}
    
    void verifyIDPresent(ColladaMeshLoader& loader);
    
    // "A scoped identifier (sid) is an xs:NCName with the added constraint that its value is unique within the
    // scope of its parent element, among the set of sids at the same path level, as found using a breadth-first
    // traveral."
    // So, this function performs a breadth-first search because Collada creators were extremely clever.
    xml_node findChildBySid(string sid);
    
    template<class T>
    T* resolveSid(string sid, ColladaMeshLoader& loader);
};

template<class T> unique_ptr<Element> new_node() { return unique_ptr<Element>(new T); }

// loader duplicates tree structure

enum class NodeAction
{
    // continue handling the subtree
    REGULAR,
    
    // delete this node & attach its subnodes to parent
    // deals with nasty 'technique_...' scopes
    DELETE_NODE,
    
    // do not recurse
    SKIP_SUBTREE
};

class ColladaMeshLoader
{
public :
    std::map<xml_node, unique_ptr<Element>> recreatedElements;
    std::map<string, xml_node> xmlNodesById;
    int nProcessed, nRecreated;
    NodeAction currentAction;
    
    string currentFile;
    
    xml_node currentNode;
    
    vector<Mesh> foundMeshes;

    ColladaMeshLoader(): nProcessed(0), nRecreated(0) {}
    
    void loadDocument(string fileName);
    
    int getNodeLineNumberSlow(xml_node node)
    {
        ptrdiff_t offset = node.offset_debug();
        
        FILE* input = fopen(currentFile.c_str(), "r");
        int lineNumber = 1;
        for (ptrdiff_t i = 0; i < offset; i++)
        {
            int c = fgetc(input);
            if (c == '\n')
                lineNumber++;
        }
        fclose(input);
        
        return lineNumber;
    }
    
    int currentNodeLineNumberSlow()
    {
        return getNodeLineNumberSlow(currentNode);
    }
    
    bool isNodeRecreated(xml_node node)
    {
        return recreatedElements.find(node) != recreatedElements.end();
    }
    
    template<class T>
    T* resolveChildLink(const char* childElementName)
    {
        return resolveChildLink<T>(childElementName, currentNode);
    }
    
    xml_node resolveChildLinkNode(const char* childElementName, xml_node node)
    {
        assert(node);
        xml_node childNode = node.child(childElementName);
        verify(childNode, "Expected child element '%s' of node '%s', line %d",
               childElementName, node.name(), currentNodeLineNumberSlow());
        
        xml_node noNext = childNode.next_sibling(childElementName);
        verify(!noNext, "Expected single element '%s' of node '%s', found second, line %d",
               childElementName, node.name(), getNodeLineNumberSlow(noNext));
        
        return childNode;
    }
    
    template<class T>
    T* resolveChildLink(const char* childElementName, xml_node node)
    {
        xml_node childNode = resolveChildLinkNode(childElementName, node);
        
        auto it = recreatedElements.find(childNode);
        if (it == recreatedElements.end())
            critical_error("Child element '%s' of node '%s' is not recreated, line %d",
                           childElementName, node.name(), getNodeLineNumberSlow(childNode));

        T* result = dynamic_cast<T*>(it->second.get());
        verify(result, "Failed to dynamic_cast child element to desired type");
    
        return result;
    }
    
    template<class T>
    T requireAttribute(const char* attributeName)
    {
        return requireAttribute<T>(attributeName, currentNode);
    }
    
    template<class T>
    T requireAttribute(const char* attributeName, xml_node node)
    {
        xml_attribute attrib = node.attribute(attributeName);
        verify(!attrib.empty(),
               "Attribute '%s' of '%s' is required but not present, line %d",
               attributeName, node.name(), getNodeLineNumberSlow(node));
        
        return castAttribute<T>(attrib);
    }
    
    template<class T>
    T castAttribute(xml_attribute attrib);
    
    xml_node resolveXmlNodeUri(string uri)
    {
        string original = uri;
        verify(!uri.empty() && uri[0] == '#', "Supported URIs must begin with '#': found '%s'", original.c_str());
        
        for (unsigned i = 1; i < uri.length(); i++)
        {
            char c = uri[i];
            
            // Ensure that we don't ignore special characters by whitelisting non-special ones
            verify(isalnum(c) || c == '_' || c == '-' || c == '.',
                   "Unsupported URI character: %c in '%s'", c, original.c_str());
        }
        
        uri = uri.substr(1, uri.length());
        
        auto it = xmlNodesById.find(uri);
        verify(it != xmlNodesById.end(), "Failed to resolve URI: '%s'", original.c_str());
        
        return it->second;
    }
    
    template<class T>
    T* resolveNodeLink(xml_node node)
    {
        auto it = recreatedElements.find(node);
        verify(it != recreatedElements.end(), "The node '%s' is not recreated (line %d)",
               node.name(), getNodeLineNumberSlow(node));
        
        T* element = dynamic_cast<T*>(it->second.get());
        verify(element, "The element for link '%s' is not of the desired type ('%s')", element, __FUNCTION__);
        
        return element;
    }
    
    template<class T>
    T* resolveUriLink(string uri)
    {
        xml_node node = resolveXmlNodeUri(uri);
        return resolveNodeLink<T>(node);
    }
    
    template<class T>
    T* resolveAttributeUriLink(const char* attributeName)
    {
        string uri = requireAttribute<string>(attributeName);
        return resolveUriLink<T>(uri);
    }
    
    unique_ptr<Element> recreateElement(xml_node node, NodeAction& action);
    
    // returns true if the node should be deleted in-place
    bool processNode(xml_node node)
    {
        if (node.type() != pugi::node_element)
            return false;
        
        nProcessed++;
        NodeAction action = NodeAction::REGULAR;
        
        if (unique_ptr<Element> thisNode = recreateElement(node, action))
        {
            thisNode->createdFrom = node;
            
            if (node.attribute("id"))
                thisNode->id = node.attribute("id").as_string();
            
            if (node.attribute("sid"))
                thisNode->sid = node.attribute("sid").as_string();
            
            currentNode = node;
            thisNode->parseFromNode(*this);
            recreatedElements[node] = move(thisNode);
            nRecreated++;
        }
        
        if (node.attribute("id"))
            xmlNodesById[node.attribute("id").as_string()] = node;
        
        if (action != NodeAction::SKIP_SUBTREE)
        {
            for (xml_node child = node.first_child(); child;)
            {
                if (!processNode(child))
                {
                    child = child.next_sibling();
                    continue;
                }
            
                // we should attach all children of child after it & remove child
                vector<xml_node> childChildren;
                
                for (xml_node childChild = child.first_child(); childChild; childChild = childChild.next_sibling())
                    childChildren.push_back(childChild);
                    
                for (xml_node childChild: childChildren)
                    verify(node.insert_move_before(childChild, child), "Failed to move node up after deletion");
                
                xml_node next = child.next_sibling();
                verify(node.remove_child(child), "Failed to remove child");
                child = next;
            }
        }
        
        if (action == NodeAction::DELETE_NODE)
            return true;
        
        return false;
    }
    
    // resolves links from children to parents, this property is used in some linking functions
    void resolveLinks(xml_node node)
    {
        for (xml_node child = node.first_child(); child; child = child.next_sibling())
            resolveLinks(child);
        
        auto recreatedElementIt = recreatedElements.find(node);
        if (recreatedElementIt == recreatedElements.end()) return;

        currentNode = recreatedElementIt->first;
        recreatedElementIt->second->resolveLinks(*this);
    }
    
    void dump(xml_node node, bool filterRecreated, int nTabs = 0)
    {
        if (node.type() != pugi::node_element)
            return;
        
        bool recreated = recreatedElements.find(node) != recreatedElements.end();
        
        if (!filterRecreated || recreated)
        {
            for (int i = 0; i < nTabs; i++)
                printf("  ");
            
            printf("%s%s\n", node.name(), recreated ? " [recreated]" : "");
        }
        
        for (xml_node child = node.first_child(); child; child = child.next_sibling())
            dump(child, filterRecreated, nTabs + 1);
    }
    
    int countNodes(xml_node node)
    {
        int count = 1;
        
        for (xml_node child = node.first_child(); child; child = child.next_sibling())
            count += countNodes(child);
        
        return count;
    }
};

template<>
int ColladaMeshLoader::castAttribute(xml_attribute attrib)
{
    int defaultInvalidValue = std::numeric_limits<int>::max();
    int value = attrib.as_int(defaultInvalidValue);
    verify(value != defaultInvalidValue, "Failed to cast attribute '%s'='%s' via 'as_int'", attrib.name(), attrib.value());
    return value;
}

template<>
double ColladaMeshLoader::castAttribute(xml_attribute attrib)
{
    double defaultInvalidValue = std::numeric_limits<double>::infinity();
    double value = attrib.as_double(defaultInvalidValue);
    verify(!std::isinf(value), "Failed to cast attribute '%s'='%s' via 'as_double'", attrib.name(), attrib.value());
    return value;
}

template<>
string ColladaMeshLoader::castAttribute(xml_attribute attrib)
{
    string defaultInvalidValue = "<invalid value>";
    string value = attrib.as_string(defaultInvalidValue.c_str());
    verify(defaultInvalidValue != value, "Failed to cast attribute '%s'='%s' via 'as_string'", attrib.name(), attrib.value());
    return value;
}

template<class T>
T* Element::resolveSid(string sid, ColladaMeshLoader& loader)
{
    xml_node child = findChildBySid(sid);
    
    return loader.resolveNodeLink<T>(child);
}

xml_node Element::findChildBySid(string sid)
{
    deque<xml_node> bfsQueue;
    bfsQueue.push_back(createdFrom);
    
    xml_node found;
    while (!bfsQueue.empty())
    {
        xml_node current = bfsQueue.front();
        bfsQueue.pop_front();
        
        if (current.attribute("sid").as_string() == sid)
        {
            found = current;
            break;
        }
        
        for (xml_node child = current.first_child(); child; child = child.next_sibling())
            bfsQueue.push_back(child);
    }
    
    verify(found, "Could not resolve scoped ID: '%s'.", sid.c_str());
    return found;
}

string tolower(string s)
{
    for (char& c: s)
        c = (char)tolower(c);
    
    return s;
}

template <class T>
void splitString(string s, vector<T>& pushTo)
{
    std::istringstream stream(s);

    T value;
    while (stream >> value)
        pushTo.push_back(value);
}

template<class T>
class Array : public Element
{
public :
    vector<T> theArray;
    
    void parseFromNode(ColladaMeshLoader& loader)
    {
        verifyIDPresent(loader);
        
        splitString<T>(loader.currentNode.child_value(), theArray);
        
        int declaredCount = loader.requireAttribute<int>("count");
        verify(theArray.size() == declaredCount,
               "Number of parsed entries (%d) does not match the declared array size (%d), at line %d.",
               theArray.size(), declaredCount, loader.currentNodeLineNumberSlow());
    }
};

// probably it is an idref array
/*
class NameArray : public Element
{
public :
    vector<Element*> theArray;
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        verifyIDPresent(loader);
        
        vector<string> namesList;
        splitString<string>(loader.currentNode.child_value(), namesList);
        
        int declaredCount = loader.requireAttribute<int>("count");
        verify(namesList.size() == declaredCount,
               "Number of parsed entries (%d) does not match the declared array size (%d), at line %d.",
               namesList.size(), declaredCount, loader.currentNodeLineNumberSlow());
        
        theArray.assign(declaredCount, nullptr);
        for (int i = 0; i < declaredCount; i++)
            theArray[i] = loader.resolveUriLink<Element>(namesList[i]);
    }
};*/

class Param : public Element
{
public :
    string name, type;
    
    void parseFromNode(ColladaMeshLoader& loader)
    {
        if (loader.currentNode.attribute("name"))
            name = loader.requireAttribute<string>("name");
        type = loader.requireAttribute<string>("type");
        
        name = tolower(name);
    }
};

class Accessor : public Element
{
public :
    Element* sourceArray;
    
    int count;
    int stride;
    vector<Param*> params;
    
    Accessor(): sourceArray(nullptr), count(0), stride(1) {}
    
    void parseFromNode(ColladaMeshLoader& loader)
    {
        count = loader.requireAttribute<int>("count");
        stride = loader.requireAttribute<int>("stride");
    }
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        string source = loader.requireAttribute<string>("source");
        sourceArray = loader.resolveUriLink<Element>(source);
        
        for (xml_node paramNode = loader.currentNode.child("param");
             paramNode; paramNode = paramNode.next_sibling("param"))
             params.push_back(loader.resolveNodeLink<Param>(paramNode));
    }
    
    int getParamOffset(string paramName, int defaultOffset = -1)
    {   
        verify(paramName == tolower(paramName),
               "Parameter name '%s' is not lower case (the search is intentionally made case-insensitive).",
               paramName.c_str());
        
        size_t nParams = params.size();
        size_t found = nParams;
        
        for (size_t i = 0; i < nParams; i++)
            if (params[i]->name == paramName)
            {
                verify(found == nParams, "Multiple parameters with name '%s' declared.", paramName.c_str());
                found = i;
            }
        
        if (defaultOffset != -1)
            verify(found != nParams, "Parameter '%s' not found.", paramName.c_str());
        
        if (found != nParams)
            return (int)found;
        else
            return defaultOffset;
    }
};

class FloatVectorAccessor
{
public :
    FloatVectorValueType resultingType;
    vector<ftype>* floatVector;
    int stride;
    int count;
    
    FloatVectorAccessor(): floatVector(nullptr), stride(0), count(0) {}
    
    void setup(Accessor* accessor)
    {   
        Array<ftype>* floatArray = dynamic_cast<Array<ftype>*>(accessor->sourceArray);
        assert(floatArray);
        
        stride = accessor->stride;
        count = accessor->count;
        
        int expectedStride = 0;
        
        if (accessor->params.size() == 1 && accessor->params[0]->type == "float4x4")
        {
            expectedStride = 16;
            resultingType = FloatVectorValueType::FLOAT_4x4;
        }
        else
        {
            expectedStride = (int)accessor->params.size();
            verify(expectedStride > 0, "At least one 'param' expected in accessor.");
            verify(expectedStride <= 4, "Maximal supported float vector is float4.");
            
            for (int i = 0; i < expectedStride; i++)
                assert(accessor->params[i]->type == "float");
            
            switch(expectedStride)
            {
                case 1: resultingType = FloatVectorValueType::FLOAT; break;
                case 2: resultingType = FloatVectorValueType::FLOAT_2; break;
                case 3: resultingType = FloatVectorValueType::FLOAT_3; break;
                case 4: resultingType = FloatVectorValueType::FLOAT_4; break;
                default: unreachable();
            }
        }
        
        verify((int)floatArray->theArray.size() == accessor->stride * accessor->count,
               "The size of float array '%s' (%d) is not equal to accessor count (%d) * stride (%d).",
               floatArray->id.c_str(),
               (int)floatArray->theArray.size(), accessor->count, accessor->stride);
        
        verify(accessor->stride == expectedStride,
               "Accessor stride to float array '%s' (%d) is not equal to expected (%d).",
               floatArray->id.c_str(), accessor->stride, expectedStride);
        
        floatVector = &floatArray->theArray;
    }
    
    void setup(Accessor* accessor, FloatVectorValueType assertType)
    {
        setup(accessor);
        verify(resultingType == assertType, "The requested type does not match accessor params.");
    }
    
    FloatVectorValue access(int index)
    {
        FloatVectorValue value;
        
        value.type = resultingType;
        int base = stride * index;
        
        for (int i = 0; i < stride; i++)
            value.subvalues.push_back((*floatVector)[base + i]);
        
        return value;
    }
    
    //template<class T>
    //T get(int index);
};

/*template<>
ftype FloatVectorAccessor::get(int index)
{
    assert(index >= 0 && index < count);
    assert(accessMode == AccessMode::FLOAT);
    
    return (*floatVector)[index * stride];
}

template<>
vec2 FloatVectorAccessor::get(int index)
{
    assert(index >= 0 && index < count);
    assert(accessMode == AccessMode::FLOAT_2);
    
    int base = index * stride;
    return vec2((*floatVector)[base + 0],
                (*floatVector)[base + 1]);
}

template<>
vec3 FloatVectorAccessor::get(int index)
{
    assert(index >= 0 && index < count);
    assert(accessMode == AccessMode::FLOAT_3);
    
    int base = index * stride;
    return vec3((*floatVector)[base + 0],
                (*floatVector)[base + 1],
                (*floatVector)[base + 2]);
}

template<>
mat4 FloatVectorAccessor::get(int index)
{
    assert(index >= 0 && index < count);
    assert(accessMode == AccessMode::FLOAT_4x4);
    
    int base = index * stride;
    
    mat4 mat;
    for (int x = 0; x < 4; x++)
        for (int y = 0; y < 4; y++)
            mat[x][y] = (*floatVector)[base + y * 4 + x];
    return mat;
}*/

class Source : public Element
{
public :
    Accessor* accessor;
    
    Source(): accessor(nullptr) {}
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        verifyIDPresent(loader);
        accessor = loader.resolveChildLink<Accessor>("accessor");
    }
};

class Input;

Input* findInputBySemantic(ColladaMeshLoader& loader, xml_node node, string semantic, bool required = false)
{
    verify(semantic == tolower(semantic),
           "Semantic '%s' is not lower case (the search is intentionally made case-insensitive).",
           semantic.c_str());
    
    xml_node found;
    
    for (xml_node input = node.child("input"); input; input = input.next_sibling("input"))
        if (tolower(input.attribute("semantic").as_string()) == semantic)
        {
            verify(!found, "Multiple inputs for semantic '%s' found as children of node '%s' (line %d)",
                   semantic.c_str(), node.name(), loader.getNodeLineNumberSlow(node));
            
            found = input;
        }
        
    if (!found)
    {
        if (!required) return nullptr;
        
        critical_error("No inputs for semantic '%s' found as child of node '%s' (line %d)",
                       semantic.c_str(), node.name(), loader.getNodeLineNumberSlow(node));
    }
    
    return loader.resolveNodeLink<Input>(found);
}

class Vertices : public Element
{
public :
    Input* position;
    vector<vector<pair<int, ftype>>> vertexWeights;
    
    Vertices(): position(nullptr) {}
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        position = findInputBySemantic(loader, loader.currentNode, "position", true);
    }
};

class Input : public Element
{
public :
    Source* generalSource;
    Vertices* verticesSource;
    
    int offset;
    
    // 'set' attribute is ignored, inputs must not share the same semantic
    string semantic;
    
    Input(): generalSource(nullptr), verticesSource(nullptr), offset(0) {}
    
    void parseFromNode(ColladaMeshLoader& loader)
    {
        if (loader.currentNode.attribute("offset"))
            offset = loader.requireAttribute<int>("offset");
        else
            offset = 0;
        
        semantic = loader.requireAttribute<string>("semantic");
        semantic = tolower(semantic);
    }
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        Element* source = loader.resolveAttributeUriLink<Element>("source");
        
        generalSource = dynamic_cast<Source*>(source);
        verticesSource = dynamic_cast<Vertices*>(source);
        
        verify((generalSource && !verticesSource) || (!generalSource && verticesSource),
               "Input source must be either a 'vertices' or a 'source' element, line %d.",
               loader.currentNodeLineNumberSlow());
    }
};

class PolylistElement : public Element
{
public :
    Input* verticesInput;
    
    vector<int> vertexCounts;
    vector<int> indices;
    
    int indexBlockSize;
    
    PolylistElement(): verticesInput(nullptr), indexBlockSize(1) {}
    
    void parseFromNode(ColladaMeshLoader& loader)
    {   
        assert(loader.currentNode.name() == string("polylist") ||
               loader.currentNode.name() == string("triangles"));
        
        int nFacesDeclared = loader.requireAttribute<int>("count");
        
        if (loader.currentNode.name() == string("polylist"))
        {
            xml_node vcount = loader.resolveChildLinkNode("vcount", loader.currentNode);
            splitString<int>(vcount.child_value(), vertexCounts);
        }
        else if (loader.currentNode.name() == string("triangles"))
        {
            vertexCounts.assign(nFacesDeclared, 3);
        }
        
        verify(nFacesDeclared == (int)vertexCounts.size(),
               "Polylist attribute 'count' does not match the number of 'vcount' entries, at line %d.",
               loader.currentNodeLineNumberSlow());
        
        xml_node pNode = loader.resolveChildLinkNode("p", loader.currentNode);
        splitString<int>(pNode.child_value(), indices);
        
        //for (int vertexCount: vertexCounts)
        //    verify(vertexCount == 3, "Non-triangle faces are not supported.");
        
        int totalVertices = accumulate(vertexCounts.begin(), vertexCounts.end(), 0);
        
        if (totalVertices != 0)
        {
            verify(indices.size() % totalVertices == 0,
                "Total indices count (%d) is not divisible by total vertices count (%d), 'polylist' at line %d.",
                (int)indices.size(), totalVertices, loader.currentNodeLineNumberSlow());
            
            indexBlockSize = (int)indices.size() / totalVertices;
            verify(indexBlockSize > 0, "At least one index per vertex is required.");
        }
        else
        {
            indexBlockSize = 1;
        }
    }
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        // NOTE: resolveLinks() resolves children links before parents
        
        verticesInput = findInputBySemantic(loader, loader.currentNode, "vertex", true);
        verify(verticesInput->verticesSource,
               "'vertex' semantic input does not have a 'vertices' source, at line %d.",
               loader.currentNodeLineNumberSlow());
        
        // NOTE: by some unknown reasons vertices node is a source of an input and also includes inputs;
        // Here it is assumed that included input has the same indices
        assert(verticesInput->offset == 0);
    }
    
    void loadPolylist(Mesh& mesh)
    {   
        Polylist newPolylist;
        
        FloatVectorAccessor vertexPositionsAccessor;
        vertexPositionsAccessor.setup
            (verticesInput->verticesSource->position->generalSource->accessor, FloatVectorValueType::FLOAT_3);
        
        int indexOffset = 0;
        
        for (int vertexCount: vertexCounts)
        {
            int baseVertexIndex = (int)mesh.vertices.size();
            
            for (int vertex = 0; vertex < vertexCount; vertex++)
            {
                // process vertex block
                
                Vertex current;
                
                int positionIndex = indices[indexOffset + verticesInput->offset];
                current.position = vertexPositionsAccessor.access(positionIndex).getValue<vec3>();
                
                indexOffset += indexBlockSize;
                
                mesh.vertices.push_back(current);
                mesh.vertexWeights.push_back(verticesInput->verticesSource->vertexWeights[positionIndex]);
            }
            
            //// NOTE: already asserted in parseFromNode
            //assert(vertexCount == 3);
            
            for (int thirdVertex = 2; thirdVertex < vertexCount; thirdVertex++)
            {
                newPolylist.indices.push_back(baseVertexIndex + 0);
                newPolylist.indices.push_back(baseVertexIndex + thirdVertex - 1);
                newPolylist.indices.push_back(baseVertexIndex + thirdVertex);
            }
        }
        
        mesh.polylists.push_back(newPolylist);
    }
};

class MeshElement : public Element
{
public :
    Vertices* vertices;
    vector<PolylistElement*> polylistElements;
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        for (const char* polylistNodeType: vector<const char*> { "polylist", "triangles" })
            for (xml_node polylistNode = loader.currentNode.child(polylistNodeType);
                polylistNode; polylistNode = polylistNode.next_sibling(polylistNodeType))
                polylistElements.push_back(loader.resolveNodeLink<PolylistElement>(polylistNode));
            
        vertices = loader.resolveChildLink<Vertices>("vertices");
    }
    
    void loadMesh(Mesh& mesh)
    {
        for (PolylistElement* polylist: polylistElements)
            polylist->loadPolylist(mesh);
    }
};

/*class MatrixElement : public Element
{
public :
    mat4 matrix;
    
    void parseFromNode(ColladaMeshLoader& loader)
    {
        vector<ftype> elements;
        splitString<ftype>(loader.currentNode.child_value(), elements);
        
        verify(elements.size() == 16,
               "'matrix' should contain 4x4 floating values (%d found), at line %d.",
               (int)elements.size(), loader.currentNodeLineNumberSlow());
        
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++)
                matrix[x][y] = elements[y * 4 + x];
    }
};*/

class ChannelElement;

class TransformElement : public Element
{
public :
    Transform transform;
    vector<ChannelElement*> attachedChannels;
    
    void parseFromNode(ColladaMeshLoader& loader)
    {
        string type = loader.currentNode.name();
        
        vector<ftype> elements;
        splitString<ftype>(loader.currentNode.child_value(), elements);
        
        static bool mapInitialized = false;
        static map<string, tuple<TransformationType, FloatVectorValueType, int>> byName;
        
        if (!mapInitialized)
        {
            byName["matrix"]    = make_tuple(TransformationType::MATRIX   , FloatVectorValueType::FLOAT_4x4, 16);
            byName["rotate"]    = make_tuple(TransformationType::ROTATE   , FloatVectorValueType::FLOAT_4  , 4);
            byName["translate"] = make_tuple(TransformationType::TRANSLATE, FloatVectorValueType::FLOAT_3  , 3);
            byName["scale"]     = make_tuple(TransformationType::SCALE    , FloatVectorValueType::FLOAT_3  , 3);
            
            byName["bind_shape_matrix"] = byName["matrix"];
            mapInitialized = true;
        }
        
        verify(byName.find(type) != byName.end(),
               "Transformation '%s' is not supported, at line %d.",
               type.c_str(), loader.currentNodeLineNumberSlow());
        
        auto typeAndCount = byName[type];
        
        verify((int)elements.size() == get<2>(typeAndCount),
               "'%s' should contain %d floating values (%d found), at line %d.",
               type.c_str(), get<2>(typeAndCount),
               (int)elements.size(), loader.currentNodeLineNumberSlow());
        
        transform.type = get<0>(typeAndCount);
        transform.value.type = get<1>(typeAndCount);
        transform.value.subvalues = elements;
    }
};

#include <ctime>

void normalize(mat4& m)
{
    /*ftype w = 0;
    for (int x = 0; x < 3; x++)
        for (int y = 0; y < 3; y++)
            w += m[x][y] * m[x][y];
    w = sqrt(w);
    for (int x = 0; x < 3; x++)
        for (int y = 0; y < 3; y++)
            m[x][y] /= w;*/
    
    for (int y = 0; y < 3; y++)
    {
        ftype w = 0;
        for (int x = 0; x < 3; x++)
            w += m[x][y] * m[x][y];
        
        w = sqrt(w);
        for (int x = 0 ; x < 3; x++)
            m[x][y] /= w;
    }
}

void Transform::applyTransform(mat4& to) const
{
    switch (type)
    {
        case TransformationType::MATRIX:
        {
            mat4 m = value.getValue<mat4>();
            normalize(m);
            m[3][3] = 1;
            to = to * m;
            break;
        }
        
        case TransformationType::ROTATE:
        {
            vec3 axis(value.subvalues[0], value.subvalues[1], value.subvalues[2]);
            to = to * glm::rotate(value.subvalues[3] / 180.0 * M_PI, axis);
            break;
        }
        
        case TransformationType::TRANSLATE:
        {
            to = to * glm::translate(value.getValue<vec3>());
            break;
        }
        
        case TransformationType::SCALE:
        {
            to = to * glm::scale(value.getValue<vec3>());
            break;
        }
        
        case TransformationType::DEBUG_ROTATE:
        {
            ftype t = (ftype)clock() / (ftype)CLOCKS_PER_SEC;
            to = to * glm::rotate(t * 45.0 / 180.0 * M_PI, vec3(0.0, 1.0, 0.0));
            break;
        }
        
        default: unreachable();
    }
}

void TransformStack::applyTransforms(mat4& to) const
{
    for (const Transform& t: transforms)
        t.applyTransform(to);
}

template<>
mat4 sge::FloatVectorValue::getValue() const
{
    assert(type == FloatVectorValueType::FLOAT_4x4);
    
    mat4 result;
    for (int x = 0; x < 4; x++)
        for (int y = 0; y < 4; y++)
            result[x][y] = subvalues[y * 4 + x];
    return result;
}

template<>
vec4 sge::FloatVectorValue::getValue() const
{
    assert(type == FloatVectorValueType::FLOAT_4);
    return vec4(subvalues[0], subvalues[1], subvalues[2], subvalues[3]);
}

template<>
vec3 sge::FloatVectorValue::getValue() const
{
    assert(type == FloatVectorValueType::FLOAT_3);
    return vec3(subvalues[0], subvalues[1], subvalues[2]);
}

template<>
vec2 sge::FloatVectorValue::getValue() const
{
    assert(type == FloatVectorValueType::FLOAT_2);
    return vec2(subvalues[0], subvalues[1]);
}

template<>
ftype sge::FloatVectorValue::getValue() const
{
    assert(type == FloatVectorValueType::FLOAT);
    return subvalues[0];
}

void sge::FloatVectorValue::setValue(const mat4& mat)
{
    assert(type == FloatVectorValueType::FLOAT_4x4);
    
    for (int x = 0; x < 4; x++)
        for (int y = 0; y < 4; y++)
            subvalues[y * 4 + x] = mat[x][y];
}


void sge::FloatVectorValue::setValue(const vec4& vec)
{
    assert(type == FloatVectorValueType::FLOAT_4);
    subvalues[0] = vec.x;
    subvalues[1] = vec.y;
    subvalues[2] = vec.z;
    subvalues[3] = vec.w;
}

void sge::FloatVectorValue::setValue(const vec3& vec)
{
    assert(type == FloatVectorValueType::FLOAT_3);
    subvalues[0] = vec.x;
    subvalues[1] = vec.y;
    subvalues[2] = vec.z;
}

void sge::FloatVectorValue::setValue(sge::ftype value)
{
    assert(type == FloatVectorValueType::FLOAT);
    subvalues[0] = value;
}

class SamplerElement : public Element
{
public :
    Input* input;
    Input* output;
    Input* interpolation;
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        input = findInputBySemantic(loader, loader.currentNode, "input", true);
        output = findInputBySemantic(loader, loader.currentNode, "output", true);
        interpolation = findInputBySemantic(loader, loader.currentNode, "interpolation", true);
    }
};

vector<string> delimiterSplit(string s, char delimiter)
{
    int n = (int)s.length();
    
    vector<string> result;

    string current = "";
    for (int i = 0; i <= n; i++)
    {
        if (i == n || s[i] == delimiter)
        {
            result.push_back(current);
            current = "";
            continue;
        }
        
        current += s[i];
    }
    
    return result;
}

class ChannelElement : public Element
{
public :
    SamplerElement* source;
    
    // target:
    bool unsupported = true;
    string fieldLookup;
    vector<int> indexPath;
    
    TransformElement* targetElement;
    int effectiveIndex;
    
    void resolveFieldIndexLookups()
    {
        TransformationType targetType = targetElement->transform.type;
     
        if (!indexPath.empty())
        {
            if (indexPath.size() == 2)
            {
                assert(targetType == TransformationType::MATRIX);
                for (int i = 0; i < 2; i++)
                    assert(indexPath[i] >= 0 && indexPath[i] <= 3);
                effectiveIndex = 4 * indexPath[1] + indexPath[0];
            }
            else if (indexPath.size() == 1)
            {
                int nSubvalues = (int)targetElement->transform.value.subvalues.size();
                assert(indexPath[0] >= 0 && indexPath[0] < nSubvalues);
                effectiveIndex = indexPath[0];
            }
            else critical_error("Invalid index path.");
        }
        else if (!fieldLookup.empty())
        {
            fieldLookup = tolower(fieldLookup);
            
            if (fieldLookup == "angle")
            {
                assert(targetType == TransformationType::ROTATE);
                effectiveIndex = 3;
            }
            else if (fieldLookup == "x" || fieldLookup == "y" || fieldLookup == "z")
            {
                assert(targetType == TransformationType::TRANSLATE ||
                       targetType == TransformationType::ROTATE ||
                       targetType == TransformationType::SCALE);
                
                effectiveIndex = fieldLookup[0] - 'x';
            }
            else critical_error("Unknown field: '%s'.", fieldLookup.c_str());
        }
        else
        {
            // interpolate the whole value
            effectiveIndex = -1;
        }
    }
    
    void parseTargetString(string target, ColladaMeshLoader& loader)
    {   
        string targetOriginal = target;
        // parse index path ...(index1)(index2)
        while (!target.empty() && target.back() == ')')
        {
            target.pop_back();
            
            int index = 0;
            int power = 1;
            while (!target.empty() && (target.back() >= '0' && target.back() <= '9'))
            {
                index += (target.back() - '0') * power;
                power *= 10;
                target.pop_back();
            }
            
            verify(!target.empty() && target.back() == '(',
                   "Failed to parse animation channel target: '%s', invalid index path.", targetOriginal.c_str());
            target.pop_back();
            
            indexPath.push_back(index);
        }
        
        reverse(indexPath.begin(), indexPath.end());
    
        // if there is a dot before '/', extract it
        size_t lastDot = target.rfind('.');
        size_t lastSlash = target.rfind('/');
        
        if (lastDot != string::npos && (lastSlash == string::npos || lastSlash < lastDot))
        {
            fieldLookup = target.substr(lastDot + 1, target.length() - lastDot - 1);
            target = target.substr(0, lastDot);
        }
        
        // split the string with slashes
        vector<string> path = delimiterSplit(target, '/');
        assert(!path.empty());
        
        // a crutch
        xml_node nowNode = loader.resolveXmlNodeUri('#' + path[0]);
        if (!loader.isNodeRecreated(nowNode))
        {
            printf("Warning: id link '%s' resolves to non-recreated element.\n", targetOriginal.c_str());
            unsupported = true;
            return;
        }
        unsupported = false;
            
        Element* now = loader.resolveNodeLink<Element>(nowNode);
        
        for (int i = 1; i < (int)path.size(); i++)
        {
            xml_node nextNode = now->findChildBySid(path[i]);
            if (!loader.isNodeRecreated(nextNode))
            {
                printf("Warning: id link '%s' resolves to non-recreated element.\n", targetOriginal.c_str());
                unsupported = true;
                return;
            }
            
            now = loader.resolveNodeLink<Element>(nextNode);
        }
        
        targetElement = dynamic_cast<TransformElement*>(now);
        verify(targetElement, "Element referenced by 'target' is not a 'transform' element.");
    }
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        source = loader.resolveAttributeUriLink<SamplerElement>("source");
        
        string target = loader.requireAttribute<string>("target");
        parseTargetString(target, loader);
        
        if (unsupported) return;
        
        resolveFieldIndexLookups();
        targetElement->attachedChannels.push_back(this);
    }
    
    void loadChannel(Mesh& mesh, int jointIndex, int transformIndex)
    {
        AnimationChannel newChannel;
        newChannel.jointIndex = jointIndex;
        newChannel.transformIndex = transformIndex;
        newChannel.subvalueIndex = effectiveIndex;
        
        int n = source->input->generalSource->accessor->count;
        
        FloatVectorAccessor timesAccessor;
        timesAccessor.setup(source->input->generalSource->accessor, FloatVectorValueType::FLOAT);
        
        FloatVectorAccessor outputAccessor;
        outputAccessor.setup(source->output->generalSource->accessor);
        
        for (int i = 0; i < n; i++)
        {
            newChannel.times.push_back(timesAccessor.access(i).getValue<ftype>());
            newChannel.values.push_back(outputAccessor.access(i));
        }
        
        mesh.animationChannels.push_back(newChannel);
    }
};

class NodeElement : public Element
{
public :
    NodeElement* parent;
    vector<NodeElement*> nodeChildren;
    vector<TransformElement*> transforms;
    int currentMeshIndex;
    
    NodeElement(): parent(nullptr) {}
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        for (xml_node nodeNode = loader.currentNode.child("node");
             nodeNode; nodeNode= nodeNode.next_sibling("node"))
        {
             nodeChildren.push_back(loader.resolveNodeLink<NodeElement>(nodeNode));
             nodeChildren.back()->parent = this;
        }
        
        for (xml_node child = loader.currentNode.first_child();
             child; child = child.next_sibling())
        {
            auto recreatedIt = loader.recreatedElements.find(child);
            if (recreatedIt == loader.recreatedElements.end()) continue;
            
            Element* element = recreatedIt->second.get();
            TransformElement* transform = dynamic_cast<TransformElement*>(element);
            
            if (transform)
                transforms.push_back(transform);
        }
        
        string type = tolower(loader.requireAttribute<string>("type"));
        verify(type == "joint" || type == "node", "Node elements must be either nodes or joints.");
    }
    
    int assignIndicesFromRoot()
    {
        int counter = 0;
        assignIndices(counter);
        return counter;
    }
    
    void assignIndices(int& indexCounter)
    {
        currentMeshIndex = indexCounter++;
        for (NodeElement* child: nodeChildren)
            child->assignIndices(indexCounter);
    }
    
    void loadJoints(Mesh& mesh)
    {
        //verify(transform, "All 'joint's must have a transform matrix.");
        
        SkeletonJoint& thisJoint = mesh.joints[currentMeshIndex];
        thisJoint.parentIndex = parent ? parent->currentMeshIndex : -1;
        
        for (TransformElement* e: transforms)
            thisJoint.transformStack.transforms.push_back(e->transform);
        
        for (NodeElement* child: nodeChildren)
            thisJoint.childrenIndices.push_back(child->currentMeshIndex);
        
        for (NodeElement* child: nodeChildren)
            child->loadJoints(mesh);
    }
    
    void loadChannels(Mesh& mesh)
    {
        for (int i = 0; i < (int)transforms.size(); i++)
            for (ChannelElement* channel: transforms[i]->attachedChannels)
                channel->loadChannel(mesh, currentMeshIndex, i);
        
        for (NodeElement* child: nodeChildren)
            child->loadChannels(mesh);
    }
};

class Geometry : public Element
{
public :
    MeshElement* meshElement;
    
    Geometry(): meshElement(nullptr) {}
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        verifyIDPresent(loader);
        meshElement = loader.resolveChildLink<MeshElement>("mesh");
    }
    
    void loadMesh(Mesh& mesh)
    {
        meshElement->loadMesh(mesh);
    }
};

class Joints : public Element
{
public :
    Input* jointInput;
    Input* invBindMatrixInput;
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        jointInput = findInputBySemantic(loader, loader.currentNode, "joint", true);
        invBindMatrixInput = findInputBySemantic(loader, loader.currentNode, "inv_bind_matrix", true);
    }
    
    void loadInverseBindMatrices(Mesh& mesh, NodeElement* skeleton, ColladaMeshLoader& loader)
    {
        Source* jointNamesSource = dynamic_cast<Source*>(jointInput->generalSource);
        assert(jointNamesSource);
        
        Array<string>* jointNames = dynamic_cast<Array<string>*>(jointNamesSource->accessor->sourceArray);
        assert(jointNames);
        
        FloatVectorAccessor invBindMatrixAccessor;
        invBindMatrixAccessor.setup(invBindMatrixInput->generalSource->accessor, FloatVectorValueType::FLOAT_4x4);
        
        assert((int)jointNames->theArray.size() == invBindMatrixAccessor.count);
        
        for (int i = 0; i < invBindMatrixAccessor.count; i++)
        {
            string jointName = jointNames->theArray[i];
            NodeElement* jointElement = skeleton->resolveSid<NodeElement>(jointName, loader);
            
            mesh.joints[jointElement->currentMeshIndex].inverseBindMatrix = invBindMatrixAccessor.access(i).getValue<mat4>();
        }
    }
};

class VertexWeights : public Element
{
public :
    Input* weightsInput;
    FloatVectorAccessor weightsAccessor;
    Input* jointsInput;
    
    vector<int> influenceCounts;
    vector<int> indices;
    
    VertexWeights(): weightsInput(nullptr), jointsInput(nullptr) {}
    
    void parseFromNode(ColladaMeshLoader& loader)
    {   
        int nVerticesDeclared = loader.requireAttribute<int>("count");
        
        xml_node vcount = loader.resolveChildLinkNode("vcount", loader.currentNode);
        splitString<int>(vcount.child_value(), influenceCounts);
    
        verify(nVerticesDeclared == (int)influenceCounts.size(),
               "vertex_weight attribute 'count' does not match the number of 'vcount' entries, at line %d.",
               loader.currentNodeLineNumberSlow());
        
        xml_node vNode = loader.resolveChildLinkNode("v", loader.currentNode);
        splitString<int>(vNode.child_value(), indices);
        
        int totalInfluences = accumulate(influenceCounts.begin(), influenceCounts.end(), 0);
        
        verify(indices.size() == 2 * totalInfluences,
            "Total indices count (%d) is not equal to influences count * 2 (%d), 'vertex_weights' at line %d.",
            (int)indices.size(), totalInfluences * 2, loader.currentNodeLineNumberSlow());
    }
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        weightsInput = findInputBySemantic(loader, loader.currentNode, "weight", true);
        jointsInput = findInputBySemantic(loader, loader.currentNode, "joint", true);
    }
    
    void loadVertexWeights(Vertices& vertices, NodeElement* skeleton, ColladaMeshLoader& loader)
    {   
        weightsAccessor.setup(weightsInput->generalSource->accessor, FloatVectorValueType::FLOAT);
        
        /*verify(influenceCounts.size() == mesh.vertices.size(),
               "Number of vertex weights (%d) is not equal to number of vertices in a mesh (%d).",
               (int)influenceCounts.size(), (int)mesh.vertices.size());
        */
        
        Array<string>* jointNames = dynamic_cast<Array<string>*>(jointsInput->generalSource->accessor->sourceArray);
        assert(jointNames);
        
        vector<int> meshJointIndices(jointNames->theArray.size());
        for (int i = 0; i < (int)jointNames->theArray.size(); i++)
        {   
            string jointName = jointNames->theArray[i];
            NodeElement* jointElement = skeleton->resolveSid<NodeElement>(jointName, loader);
            
            meshJointIndices[i] = jointElement->currentMeshIndex;
        }
        
        int indexOffset = 0;
        
        vertices.vertexWeights.resize(influenceCounts.size());
        
        for (int vertex = 0; vertex < (int)influenceCounts.size(); vertex++)
        {
            int influenceCount = influenceCounts[vertex];
            
            ftype totalWeight = 0;
            
            for (int influenceIndex = 0; influenceIndex < influenceCount; influenceIndex++)
            {
                int jointIndicesIndex = indices[indexOffset + jointsInput->offset];
                int weightIndex = indices[indexOffset + weightsInput->offset];
                
                assert(jointIndicesIndex >= 0 && jointIndicesIndex < (int)meshJointIndices.size());
                
                int meshJointIndex = meshJointIndices[jointIndicesIndex];
                ftype weight = weightsAccessor.access(weightIndex).getValue<ftype>();
                totalWeight += weight;
                
                vertices.vertexWeights[vertex].push_back(make_pair(meshJointIndex, weight));
                
                indexOffset += 2;
            }
            
            for (auto& weightPair: vertices.vertexWeights[vertex])
                weightPair.second /= totalWeight;
        }
    }
};

class Skin : public Element
{
public :
    Geometry* baseGeometry;
    Joints* joints;
    VertexWeights* vertexWeights;
    mat4 bindShapeMatrix;
    
    Skin(): baseGeometry(nullptr) {}
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        baseGeometry = loader.resolveAttributeUriLink<Geometry>("source");
        TransformElement* bindShapeMatrixElement
            = loader.resolveChildLink<TransformElement>("bind_shape_matrix");
            
        bindShapeMatrix = bindShapeMatrixElement->transform.value.getValue<mat4>();
        
        joints = loader.resolveChildLink<Joints>("joints");
        vertexWeights = loader.resolveChildLink<VertexWeights>("vertex_weights");
    }
    
    void loadMesh(Mesh& mesh)
    {
        mesh.bindShapeMatrix = bindShapeMatrix;
        baseGeometry->loadMesh(mesh);
    }
};

class Controller : public Element
{
public :
    Skin* skin;
    
    Controller(): skin(nullptr) {}
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        if (loader.currentNode.child("skin"))
            skin = loader.resolveChildLink<Skin>("skin");
    }
    
    void loadMesh(Mesh& mesh)
    {
        verify(skin, "Loading of non-'skin' controllers is not implemented.");
        skin->loadMesh(mesh);
    }
};

class InstanceController : public Element
{
public :
    Controller* toInstance;
    NodeElement* skeleton;
    NodeElement* armature;
    
    InstanceController(): toInstance(nullptr) {}
    
    void resolveLinks(ColladaMeshLoader& loader)
    {
        string uri = loader.requireAttribute<string>("url");
        toInstance = loader.resolveUriLink<Controller>(uri);
        
        // FIXME: there can be multiple skeleton hints, ...
        
        xml_node skeletonNode = loader.currentNode.child("skeleton");
        verify(skeletonNode, "Loading of 'instance_controller' nodes without a 'skeleton' is not implemented.");
        
        string skeletonUri = skeletonNode.child_value();
        xml_node skeletonRootJoint = loader.resolveXmlNodeUri(skeletonUri);
        xml_node armatureNode = skeletonRootJoint.parent();
        
        skeleton = loader.resolveNodeLink<NodeElement>(skeletonRootJoint);
        armature = loader.resolveNodeLink<NodeElement>(armatureNode);
    }
    
    void loadMesh(Mesh& mesh, ColladaMeshLoader& loader)
    {
        int nJoints = skeleton->assignIndicesFromRoot();
        mesh.joints.resize(nJoints);
        
        for (TransformElement* e: armature->transforms)
            mesh.armatureTransformStack.transforms.push_back(e->transform);
        
        skeleton->loadJoints(mesh);
        skeleton->loadChannels(mesh);
        toInstance->skin->joints->loadInverseBindMatrices(mesh, skeleton, loader);
        toInstance->skin->vertexWeights->loadVertexWeights(*toInstance->skin->baseGeometry->meshElement->vertices, skeleton, loader);
        
        toInstance->loadMesh(mesh);
    }
};

void Element::verifyIDPresent(ColladaMeshLoader& loader)
{
    verify(!id.empty(), "All elements of type '%s' must have an ID (line %d)",
           loader.currentNode.name(), loader.currentNodeLineNumberSlow());
}

unique_ptr<Element> ColladaMeshLoader::recreateElement(xml_node node, NodeAction& action)
{
    string type = tolower(node.name());
    
    if (type.find("technique") == 0 || type.find("profile") == 0)
    {
        action = NodeAction::DELETE_NODE;
        return nullptr;
    }
    
    if (type == "sampler2d")
    {
        action = NodeAction::SKIP_SUBTREE;
        return nullptr;
    }
    
#define matchName(s, ClassName) if (type == s) return new_node<ClassName>();

    matchName("float_array", Array<ftype>);
    matchName("idref_array", Array<string>);
    matchName("name_array", Array<string>);
    matchName("accessor", Accessor);
    matchName("param", Param);
    matchName("source", Source);
    matchName("controller", Controller);
    matchName("skin", Skin);
    matchName("instance_controller", InstanceController);
    matchName("geometry", Geometry);
    matchName("mesh", MeshElement);
    matchName("input", Input);
    matchName("vertices", Vertices);
    matchName("polylist", PolylistElement);
    matchName("triangles", PolylistElement);
    matchName("node", NodeElement);
    matchName("matrix", TransformElement);
    matchName("scale", TransformElement);
    matchName("rotate", TransformElement);
    matchName("translate", TransformElement);
    matchName("bind_shape_matrix", TransformElement);
    matchName("joints", Joints);
    matchName("vertex_weights", VertexWeights);
    matchName("channel", ChannelElement);
    matchName("sampler", SamplerElement);
    
    // NOTE: unsupported, just to ensure there are no such elements
    matchName("skew", TransformElement);
    matchName("lookat", TransformElement);
    
#undef matchName
    
    return nullptr;
}

void ColladaMeshLoader::loadDocument(string fileName)
{
    currentFile = fileName;
    
    xml_document document;
    xml_parse_result parseResult = document.load_file(fileName.c_str());
    
    if (!parseResult)
        critical_error("Failed to parse XML file '%s': %s\n", fileName.c_str(), parseResult.description());
    
    SDL_assert(parseResult);
    
    xml_node rootNode = document.child("COLLADA");
    
#if 0
    printf("All nodes:\n");
    dump(rootNode, false);
#endif

    processNode(rootNode);
    resolveLinks(rootNode);

    /*
    printf("All nodes:\n");
    dump(rootNode, false);
    printf("Recreated nodes:\n");
    dump(rootNode, true);
    */
    
    int totalNodes = countNodes(rootNode);
    printf("%d nodes total, %d nodes processed, %d recreated\n", totalNodes, nProcessed, nRecreated);
    
    currentFile = "";
    
    struct FindInstanceControllerPredicate
    {
        bool operator() (const xml_node& node) const
        {
            return node.name() == string("instance_controller");
        }
    };
    
    xml_node instanceController = document.find_node(FindInstanceControllerPredicate());
    verify(instanceController, "Temp: COLLADA file is expected to have an instance_controller node.");
    
    foundMeshes.resize(foundMeshes.size() + 1);
    
    InstanceController* controllerRecreated = resolveNodeLink<InstanceController>(instanceController);
    controllerRecreated->loadMesh(foundMeshes.back(), *this);
}

Mesh sge::loadColladaMeshNew(string fileName)
{
    ColladaMeshLoader loader;
    loader.loadDocument(fileName);
    
    verify(loader.foundMeshes.size() == 1, "Should've loaded a single mesh.");
    return loader.foundMeshes[0];
}

void Mesh::slowRender()
{   
    applyAnimation();
    
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);
    
    GLint wasMode[2];
    glGetIntegerv(GL_POLYGON_MODE, wasMode);
    
    glColor3d(1, 1, 1);
    glPolygonOffset(0, 0);
    slowRenderPass(wasMode[0] == GL_LINE);
    
    glLineWidth(2);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3d(0, 0, 0);
    glPolygonOffset(-2, -2);
    slowRenderPass(wasMode[0] == GL_LINE);
    glLineWidth(1);
    
    glPolygonMode(GL_FRONT, wasMode[0]);
    glPolygonMode(GL_BACK, wasMode[1]);
    
    
    if (!skinned)
    {
        //skinned = true;
        applySkinning();
    }
}

void Mesh::slowRenderPass(bool /*skeletonOnly*/)
{
    //printf("slow render %d polylists\n", polylists.size());
    
    mat4 root;
    armatureTransformStack.applyTransforms(root);
        
    joints[0].slowRender(*this, root, renderSkeleton);
    
    static bool junkied = false;
    if (!junkied)
    {
        junkied = true;
        //joints[5].transformStack.transforms.push_back(Transform { TransformationType::DEBUG_ROTATE });
        //armatureTransformStack.transforms.push_back(Transform { TransformationType::DEBUG_ROTATE });
    }
    
    static bool premultiplied = false;
    
    if (!premultiplied)
    {
        premultiplied = true;
        for (auto& it: vertices)
            it.position = vec3(bindShapeMatrix * vec4(it.position, 1));
    }
    
    //if (!skeletonOnly)
    //if (false)
        for (Polylist& p: polylists)
            p.slowRender(vertices);
}

void interpolate(const FloatVectorValue& a, const FloatVectorValue& b, FloatVectorValue& result, ftype t)
{
    assert(a.subvalues.size() == b.subvalues.size() && a.subvalues.size() == result.subvalues.size());
    int n = (int)a.subvalues.size();
    
    //printf("interpolate %f..%f with %f\n", a.subvalues[0], b.subvalues[0], t);
    
    for (int i = 0; i < n; i++)
        result.subvalues[i] = a.subvalues[i] * (1 - t) + b.subvalues[i] * t;
}

void AnimationChannel::applyValue(Mesh& to, sge::ftype t)
{
    //t = 0.0;
    /*if (subvalueIndex == -1)
    {
        printf("Subvalue index -1!!\n");
        return;
    }*/
    
    ftype span = times.back() - times.front();
    
    //while (t > span * 33 / 36.0)
    //    t -= 32 / 36.0 * span;
    
    while (t > times.back()) t -= span;
    while (t < times.front()) t += span;
    
    bool found = false;
    for (int i = 0; i < (int)times.size() - 1; i++)
        if (times[i] <= t + FTYPE_WEAK_EPS && times[i + 1] >= t - FTYPE_WEAK_EPS)
        {
            ftype tInterp = (t - times[i]) / (times[i + 1] - times[i]);
            
            if (subvalueIndex != -1)
            {
                ftype interpolated = values[i].subvalues[0] * (1 - tInterp) + values[i + 1].subvalues[0] * tInterp;
                to.joints[jointIndex].transformStack.transforms[transformIndex].value.subvalues[subvalueIndex] = interpolated;
            }
            else
            {
                
                interpolate(values[i], values[i + 1], to.joints[jointIndex].transformStack.transforms[transformIndex].value, tInterp);
            }
            
            found = true;
            break;
        }
    assert(found);
}

void Mesh::applyAnimation()
{
    ftype t = (ftype)clock() / (ftype)CLOCKS_PER_SEC;
    //t *= 0.01;
    //t = 0;
    
    //for (AnimationChannel& channel: animationChannels)
    for (int i = 0; i < min((int)animationChannels.size(), 9 * 5 * 100); i++)
    {
        AnimationChannel& channel = animationChannels[i];
        channel.applyValue(*this, t);
    }
    //exit(0);
}

void Mesh::applySkinning()
{
    
    int nWeightings = 0;
    for (int i = 0; i < (int)vertices.size(); i++)
    {
        vec4 v = vec4(vertices[i].position, 1);
        vec3& result = vertices[i].skinnedPosition;
        
        result = vec3();
        for (int j = 0; j < (int)vertexWeights[i].size(); j++)
        {
            int jointIndex = vertexWeights[i][j].first;
            ftype weight = vertexWeights[i][j].second;
            nWeightings++;
            result += vec3(joints[jointIndex].transformationMatrix /* joints[jointIndex].inverseBindMatrix*/ /* bindShapeMatrix*/ * v * weight);
        }
    }
    
    //printf("%d vertices %d weightings, %g avg\n", (int)vertices.size(), nWeightings, nWeightings / (ftype)vertices.size());
}

void Polylist::slowRender(vector<Vertex>& vertices)
{
    //printf("slow render %d vertices %d indices\n", vertices.size(), indices.size());
    
    glBindTexture(GL_TEXTURE_2D, material.diffuseTexture.loadedId);
    
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < indices.size(); i++)
    {
        vertices[indices[i]].slowRender();
    }
    glEnd();
}

void Vertex::slowRender()
{
    glVertex3d(skinnedPosition.x, skinnedPosition.y, skinnedPosition.z);
}

void dumpRenderCube(mat4 transform)
{
    std::vector<vec3> vertices =
        { { -1, -1, -1 }, { -1, -1, +1 },
          { -1, +1, -1 }, { -1, +1, +1 }, 
          { +1, -1, -1 }, { +1, -1, +1 },
          { +1, +1, -1 }, { +1, +1, +1 } };
          
    for (vec3& v: vertices)
        v = vec3(transform * vec4(v, 1));
    
    vector<int> indices;
    
#define quad(a, b, c, d) \
    { \
        indices.push_back(a); indices.push_back(b); indices.push_back(c); \
        indices.push_back(a); indices.push_back(c); indices.push_back(d); \
    }
    
    quad(0, 1, 3, 2);
    quad(4, 5, 7, 6);
    
    quad(0, 1, 5, 4);
    quad(2, 3, 7, 6);
    
    quad(0, 2, 6, 4);
    quad(1, 3, 7, 5);
    
#undef quad
    
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < indices.size(); i++)
    {
        vec3 v = vertices[indices[i]];
        glVertex3d(v.x, v.y, v.z);
    }
    glEnd();
}

void SkeletonJoint::slowRender(Mesh& mesh, mat4 parentTransform, bool renderSkeleton)
{
#if 0
    for (int x = 0; x < 4; x++)
    {
        for (int y = 0; y < 4; y++)
            printf("%f ", transformMatrix[x][y]);
        printf("\n");
    }
#endif
    
    mat4 scaleMatrix = mat4();
    scaleMatrix = glm::scale(scaleMatrix, vec3(0.1, 0.1, 0.1));
    
    if (renderSkeleton)
        dumpRenderCube(parentTransform * scaleMatrix);

    transformStack.applyTransforms(parentTransform);
    transformationMatrix = parentTransform * inverseBindMatrix;
    
    /*glm::vec4 zero(0, 0, 0, 1);
    zero = zero * parentTransform * transformMatrix;
    printf("position %f %f %f\n", zero.x, zero.y, zero.z);
    */
    
    for (int childIndex: childrenIndices)
        mesh.joints[childIndex].slowRender(mesh, parentTransform, renderSkeleton);
}
