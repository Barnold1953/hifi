//
//  OctreeElement.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//
//

#ifndef __hifi__OctreeElement__
#define __hifi__OctreeElement__

//#define HAS_AUDIT_CHILDREN
//#define SIMPLE_CHILD_ARRAY
#define SIMPLE_EXTERNAL_CHILDREN

#include <QReadWriteLock>

#include <SharedUtil.h>
#include "AABox.h"
#include "ViewFrustum.h"
#include "OctreeConstants.h"
//#include "Octree.h"

class Octree;
class OctreeElement;
class OctreeElementDeleteHook;
class OctreePacketData;
class VoxelSystem;
class ReadBitstreamToTreeParams;

// Callers who want delete hook callbacks should implement this class
class OctreeElementDeleteHook {
public:
    virtual void elementDeleted(OctreeElement* element) = 0;
};

// Callers who want update hook callbacks should implement this class
class OctreeElementUpdateHook {
public:
    virtual void elementUpdated(OctreeElement* element) = 0;
};


class OctreeElement {

protected:
    // can only be constructed by derived implementation
    OctreeElement();

    virtual OctreeElement* createNewElement(unsigned char * octalCode = NULL) const = 0;
    
public:
    virtual void init(unsigned char * octalCode); /// Your subclass must call init on construction.
    virtual ~OctreeElement();
    
    const unsigned char* getOctalCode() const { return (_octcodePointer) ? _octalCode.pointer : &_octalCode.buffer[0]; }
    OctreeElement* getChildAtIndex(int childIndex) const;
    void deleteChildAtIndex(int childIndex);
    OctreeElement* removeChildAtIndex(int childIndex);
    virtual OctreeElement* addChildAtIndex(int childIndex);
    void safeDeepDeleteChildAtIndex(int childIndex, int recursionCount = 0); // handles deletion of all descendents

    virtual void calculateAverageFromChildren() { };
    virtual bool collapseChildren() { return false; };

    const AABox& getAABox() const { return _box; }
    const glm::vec3& getCorner() const { return _box.getCorner(); }
    float getScale() const { return _box.getScale(); }
    int getLevel() const { return numberOfThreeBitSectionsInCode(getOctalCode()) + 1; }
    
    float getEnclosingRadius() const;

    virtual bool hasContent() const { return isLeaf(); }
    virtual void splitChildren() { }
    virtual bool requiresSplit() const { return false; }
    virtual bool appendElementData(OctreePacketData* packetData) const { return true; }
    virtual int readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) 
                    { return 0; }

    bool isInView(const ViewFrustum& viewFrustum) const; 
    ViewFrustum::location inFrustum(const ViewFrustum& viewFrustum) const;
    float distanceToCamera(const ViewFrustum& viewFrustum) const; 
    float furthestDistanceToCamera(const ViewFrustum& viewFrustum) const;

    bool calculateShouldRender(const ViewFrustum* viewFrustum, 
                float voxelSizeScale = DEFAULT_OCTREE_SIZE_SCALE, int boundaryLevelAdjust = 0) const;
    
    // points are assumed to be in Voxel Coordinates (not TREE_SCALE'd)
    float distanceSquareToPoint(const glm::vec3& point) const; // when you don't need the actual distance, use this.
    float distanceToPoint(const glm::vec3& point) const;

    bool isLeaf() const { return getChildCount() == 0; }
    int getChildCount() const { return numberOfOnes(_childBitmask); }
    void printDebugDetails(const char* label) const;
    bool isDirty() const { return _isDirty; }
    void clearDirtyBit() { _isDirty = false; }
    void setDirtyBit() { _isDirty = true; }
    bool hasChangedSince(uint64_t time) const { return (_lastChanged > time); }
    void markWithChangedTime();
    uint64_t getLastChanged() const { return _lastChanged; }
    void handleSubtreeChanged(Octree* myTree);
    
    // Used by VoxelSystem for rendering in/out of view and LOD
    void setShouldRender(bool shouldRender);
    bool getShouldRender() const { return _shouldRender; }
    
    /// we assume that if you should be rendered, then your subclass is rendering, but this allows subclasses to
    /// implement alternate rendering strategies
    virtual bool isRendered() const { return getShouldRender(); }
    
    void setSourceUUID(const QUuid& sourceID);
    QUuid getSourceUUID() const;
    uint16_t getSourceUUIDKey() const { return _sourceUUIDKey; }
    bool matchesSourceUUID(const QUuid& sourceUUID) const;
    static uint16_t getSourceNodeUUIDKey(const QUuid& sourceUUID);

    static void addDeleteHook(OctreeElementDeleteHook* hook);
    static void removeDeleteHook(OctreeElementDeleteHook* hook);

    static void addUpdateHook(OctreeElementUpdateHook* hook);
    static void removeUpdateHook(OctreeElementUpdateHook* hook);
    
    static unsigned long getNodeCount() { return _voxelNodeCount; }
    static unsigned long getInternalNodeCount() { return _voxelNodeCount - _voxelNodeLeafCount; }
    static unsigned long getLeafNodeCount() { return _voxelNodeLeafCount; }

    static uint64_t getVoxelMemoryUsage() { return _voxelMemoryUsage; }
    static uint64_t getOctcodeMemoryUsage() { return _octcodeMemoryUsage; }
    static uint64_t getExternalChildrenMemoryUsage() { return _externalChildrenMemoryUsage; }
    static uint64_t getTotalMemoryUsage() { return _voxelMemoryUsage + _octcodeMemoryUsage + _externalChildrenMemoryUsage; }

    static uint64_t getGetChildAtIndexTime() { return _getChildAtIndexTime; }
    static uint64_t getGetChildAtIndexCalls() { return _getChildAtIndexCalls; }
    static uint64_t getSetChildAtIndexTime() { return _setChildAtIndexTime; }
    static uint64_t getSetChildAtIndexCalls() { return _setChildAtIndexCalls; }

#ifdef BLENDED_UNION_CHILDREN
    static uint64_t getSingleChildrenCount() { return _singleChildrenCount; }
    static uint64_t getTwoChildrenOffsetCount() { return _twoChildrenOffsetCount; }
    static uint64_t getTwoChildrenExternalCount() { return _twoChildrenExternalCount; }
    static uint64_t getThreeChildrenOffsetCount() { return _threeChildrenOffsetCount; }
    static uint64_t getThreeChildrenExternalCount() { return _threeChildrenExternalCount; }
    static uint64_t getCouldStoreFourChildrenInternally() { return _couldStoreFourChildrenInternally; }
    static uint64_t getCouldNotStoreFourChildrenInternally() { return _couldNotStoreFourChildrenInternally; }
#endif

    static uint64_t getExternalChildrenCount() { return _externalChildrenCount; }
    static uint64_t getChildrenCount(int childCount) { return _childrenCount[childCount]; }
    
#ifdef BLENDED_UNION_CHILDREN
#ifdef HAS_AUDIT_CHILDREN
    void auditChildren(const char* label) const;
#endif // def HAS_AUDIT_CHILDREN
#endif // def BLENDED_UNION_CHILDREN

protected:
    void deleteAllChildren();
    void setChildAtIndex(int childIndex, OctreeElement* child);

#ifdef BLENDED_UNION_CHILDREN
    void storeTwoChildren(OctreeElement* childOne, OctreeElement* childTwo);
    void retrieveTwoChildren(OctreeElement*& childOne, OctreeElement*& childTwo);
    void storeThreeChildren(OctreeElement* childOne, OctreeElement* childTwo, OctreeElement* childThree);
    void retrieveThreeChildren(OctreeElement*& childOne, OctreeElement*& childTwo, OctreeElement*& childThree);
    void decodeThreeOffsets(int64_t& offsetOne, int64_t& offsetTwo, int64_t& offsetThree) const;
    void encodeThreeOffsets(int64_t offsetOne, int64_t offsetTwo, int64_t offsetThree);
    void checkStoreFourChildren(OctreeElement* childOne, OctreeElement* childTwo, OctreeElement* childThree, OctreeElement* childFour);
#endif
    void calculateAABox();
    void notifyDeleteHooks();
    void notifyUpdateHooks();

    AABox _box; /// Client and server, axis aligned box for bounds of this voxel, 48 bytes

    /// Client and server, buffer containing the octal code or a pointer to octal code for this node, 8 bytes
    union octalCode_t {
      unsigned char buffer[8];
      unsigned char* pointer;
    } _octalCode;  

    uint64_t _lastChanged; /// Client and server, timestamp this node was last changed, 8 bytes

    /// Client and server, pointers to child nodes, various encodings
#ifdef SIMPLE_CHILD_ARRAY
    OctreeElement* _simpleChildArray[8]; /// Only used when SIMPLE_CHILD_ARRAY is enabled
#endif

#ifdef SIMPLE_EXTERNAL_CHILDREN
    union children_t {
      OctreeElement* single;
      OctreeElement** external;
    } _children;
#endif
    
#ifdef BLENDED_UNION_CHILDREN
    union children_t {
      OctreeElement* single;
      int32_t offsetsTwoChildren[2];
      uint64_t offsetsThreeChildrenEncoded;
      OctreeElement** external;
    } _children;
#ifdef HAS_AUDIT_CHILDREN
    OctreeElement* _childrenArray[8]; /// Only used when HAS_AUDIT_CHILDREN is enabled to help debug children encoding
#endif // def HAS_AUDIT_CHILDREN

#endif //def BLENDED_UNION_CHILDREN

    uint16_t _sourceUUIDKey; /// Client only, stores node id of voxel server that sent his voxel, 2 bytes

    // Support for _sourceUUID, we use these static member variables to track the UUIDs that are
    // in use by various voxel server nodes. We map the UUID strings into an 16 bit key, this limits us to at
    // most 65k voxel servers in use at a time within the client. Which is far more than we need.
    static uint16_t _nextUUIDKey; // start at 1, 0 is reserved for NULL
    static std::map<QString, uint16_t> _mapSourceUUIDsToKeys;
    static std::map<uint16_t, QString> _mapKeysToSourceUUIDs;

    unsigned char _childBitmask;     // 1 byte 

    bool _falseColored : 1, /// Client only, is this voxel false colored, 1 bit
         _isDirty : 1, /// Client only, has this voxel changed since being rendered, 1 bit
         _shouldRender : 1, /// Client only, should this voxel render at this time, 1 bit
         _octcodePointer : 1, /// Client and Server only, is this voxel's octal code a pointer or buffer, 1 bit
         _unknownBufferIndex : 1,
         _childrenExternal : 1; /// Client only, is this voxel's VBO buffer the unknown buffer index, 1 bit

    static QReadWriteLock _deleteHooksLock;
    static std::vector<OctreeElementDeleteHook*> _deleteHooks;

    //static QReadWriteLock _updateHooksLock;
    static std::vector<OctreeElementUpdateHook*> _updateHooks;

    static uint64_t _voxelNodeCount;
    static uint64_t _voxelNodeLeafCount;

    static uint64_t _voxelMemoryUsage;
    static uint64_t _octcodeMemoryUsage;
    static uint64_t _externalChildrenMemoryUsage;

    static uint64_t _getChildAtIndexTime;
    static uint64_t _getChildAtIndexCalls;
    static uint64_t _setChildAtIndexTime;
    static uint64_t _setChildAtIndexCalls;

#ifdef BLENDED_UNION_CHILDREN
    static uint64_t _singleChildrenCount;
    static uint64_t _twoChildrenOffsetCount;
    static uint64_t _twoChildrenExternalCount;
    static uint64_t _threeChildrenOffsetCount;
    static uint64_t _threeChildrenExternalCount;
    static uint64_t _couldStoreFourChildrenInternally;
    static uint64_t _couldNotStoreFourChildrenInternally;
#endif
    static uint64_t _externalChildrenCount;
    static uint64_t _childrenCount[NUMBER_OF_CHILDREN + 1];
};

#endif /* defined(__hifi__OctreeElement__) */