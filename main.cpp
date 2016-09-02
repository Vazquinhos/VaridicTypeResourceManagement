#include <stdio.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <assert.h>

#if defined(DEBUG) || defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <typeinfo>

union vec3
{
  float u[3];
  struct
  { 
    float x;
    float y;
    float z;
  };
};

struct CTexture
{
  size_t mID;
  size_t mWidth;
  size_t mHeight;
};

struct CMesh
{
  std::vector<float>    mVertices;
  std::vector< size_t > mIndices;
};

namespace Serialization
{
  template < typename T > T* Load(const std::string&);
  template < typename T > void Store( const std::string& );

  template <> CTexture* Load< CTexture >(const std::string& aFileName)
  {
    CTexture* lTexture = new CTexture();
    lTexture->mID = 0;
    lTexture->mWidth  = 4096;
    lTexture->mHeight = 4096;
    return lTexture;
  }

  template <> CMesh* Load< CMesh >(const std::string& aFileName)
  {
    CMesh* lMesh = new CMesh();
    lMesh->mVertices.push_back(5.0);
    lMesh->mIndices.push_back(10);
    return lMesh;
  }
}

class CResource
{
public:
  CResource() : mData( nullptr ), mRefCount(1) {}
  virtual ~CResource(){}
  void AddRef(){++mRefCount;}
  void RemoveRef(){--mRefCount;}
  template< typename T >
  T* Get() { return static_cast< T* >( mData ); }

  template< typename T >
  void Load( const std::string& aFileName )
  {
    assert(!mData);
    mData = Serialization::Load<T>(aFileName);
  }

private:
  size_t mRefCount;
  void* mData;
};

class CResourceManager
{
public:
  CResourceManager(){}
  virtual ~CResourceManager(){}

  template< typename T >
  void Register() { mRepository[typeid(T).hash_code()] = TBranch(); }

  template< typename T > 
  T* Get( const std::string& aFileName )
  {
    T* lObject = nullptr;
    if( !aFileName.empty() )
    {
      assert( mRepository.find(typeid(T).hash_code()) != mRepository.end() );

      TBranch& lTypeBranch = mRepository.find(typeid(T).hash_code())->second;
      TBranch::iterator lItFindBranch = lTypeBranch.find(aFileName);
      if( lItFindBranch != lTypeBranch.end() )
      {
        CResource& lResource = lItFindBranch->second;
        lObject = lResource.Get<T>();
        lResource.AddRef();
      }
      else
      {
        CResource lResource;
        lResource.Load<T>(aFileName);
        lObject = lResource.Get<T>();
        lTypeBranch[aFileName] = lResource;
      }
    }
    return lObject;
  }

private:
  typedef std::map< std::string, CResource > TBranch;
  typedef std::map< size_t , TBranch > TRepository;
  TRepository mRepository;
};

class CAttributesTable
{
private:
  template<typename T>
  class AnyTypedHolder
  {
    public:
      AnyTypedHolder(const T& value) : mVal(value), mTypeInfo(typeid(T).hash_code()) {}
      size_t                TypeHashCode() const { return mTypeInfo; }
      std::type_info const& Type() const { return typeid(T); }
      size_t mTypeInfo;
      T mVal;
  };

  typedef std::map< std::string, void* > TAttributesMap;
  TAttributesMap mData;
public:
  CAttributesTable(){}
  virtual ~CAttributesTable()
  {
    for(TAttributesMap::iterator lItb = mData.begin(), lIte = mData.end(); lItb != lIte; ++lItb )
      delete( lItb->second );
  }

  template < typename T >
  void Add(const std::string& aName, const T& aData )
  {
    mData[aName] = new AnyTypedHolder<T>(aData);
  }

  template < typename T >
  void Remove(const std::string& aName, const T& aData )
  {
    TAttributesMap::const_iterator lItFind = mData.find(aName);
    if(lItFind != mData.end() )
    {
      delete( lItb->second );
      mData.erase(lItFind);
    }
  }

  template < typename T >
  bool Get( const std::string& aName, T& aData ) const
  {
    bool lOk = false;
    TAttributesMap::const_iterator lItFind = mData.find(aName);
    if(lItFind != mData.end() )
    {
      AnyTypedHolder<T>* lAuxPtr = static_cast<AnyTypedHolder<T>*>(lItFind->second);
      assert( lAuxPtr->TypeHashCode() == typeid(T).hash_code() );
      aData = lAuxPtr->mVal;
      lOk = true;
    }
    return lOk;
  }
};

void main()
{
  /*
  int size = std::rand();
  std::vector< char > lData(size * 3 * sizeof(float), 1);
  std::vector< float > lPositions( size*3, std::rand() );

  char* lDataItr = &lData[0];

  for( int i = 0; i < size * 3; ++i)
    lPositions[i] = float( std::rand() );

  for( int i = 0; i < size; i+=3, lDataItr += 3 * sizeof(float) )
    memcpy(lDataItr,&lPositions[i], 3* sizeof(float));

  std::vector< float > lPositionsCheck( size*3, 0 );
  memcpy(&lPositionsCheck[0], &lData[0], size * 3 * sizeof(float) );
  */

  CResourceManager lResManager;
  lResManager.Register<CTexture>();
  lResManager.Register<CMesh>();
  CTexture* lTexture = lResManager.Get<CTexture>("hola");
  CMesh* lMesh = lResManager.Get<CMesh>("2");

  assert( lMesh->mVertices.size() == 1);
  assert( lTexture->mWidth == 4096 );
}

/*
void main()
  /*
#if defined(DEBUG) | defined(_DEBUG)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //_CrtSetBreakAlloc(3837);
#endif
  vec3 * pa = new vec3();
  vec3 a;
  memset(&a, 0, sizeof(vec3));

  CAttributesTable lTable;
  lTable.Add("position", a);

  vec3 a2;
  memset(&a2, 1, sizeof(vec3));
  
  lTable.Get<vec3>("position", a2 );

  float l1 = 5;
  float l2 = 6;

  lTable.Add<float>("health", l1);
  lTable.Add<float>("mana", l2);

  float h, m;
  float h = lTable.Get("health" );
  lTable.Get<float>("mana", m );

  printf( "Position %f| %f| %f - Health %f - Mana %f", a.x, a.y, a.z, h, m );

  // This will crash becaus the type stored into the map for "position" is vec3
  lTable.Get<float>("position", l1 );
}*/


