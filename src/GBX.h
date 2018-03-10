#ifndef GBX_H
#define GBX_H

//
//   ______________________  ___
//  /  _____/\______   \   \/  /
// /   \  ___ |    |  _/\     / 
// \    \_\  \|    |   \/     \ 
//  \______  /|______  /___/\  \
//         \/        \/      \_/
//                         v0.1.0
//
// (c) 2018 Joel "ZappedCow" Lauener
//

#define DEFAULT_FRAME_RATE 30

#define TYPES_INITIAL_CAPACITY 5
#define LAYERS_INITIAL_CAPACITY 5
#define RENDERABLES_BY_LAYER_INITIAL_CAPACITY 5

#include <Gamebuino-Meta.h> // FIXME should be only included in cpp

//-----------------------------------------------------------------------------
// PtrVector
//-----------------------------------------------------------------------------

template<class T>
class PtrVector
{
public:
  PtrVector<T>(uint16_t initialCapacity, bool ownData = false) :
    capacity(initialCapacity),
    ownData(ownData)
  {
  }

  virtual ~PtrVector()
  {
    if (data != NULL)
    {
      if (ownData)
      {
        for (uint16_t i = 0; i < size; i++)
        {
          if (data[i] != NULL)
          {
            free(data[i]);
          }
        }
      }
      free(data);
    }
  }

  T*& operator[](uint16_t index)
  {
    while (size <= index)
    {
      add(NULL);
    }
    return data[index];
  }

  void add(T* element)
  {
    if (data == NULL)
    {
      data = (T**)malloc(capacity * sizeof(T));
    }

    if (size == capacity)
    {
      // TODO check for alloc error
      data = (T**)realloc(data, capacity * 2 * sizeof(T));
      capacity *= 2;
    }

    data[size++] = element;
  }

  void remove(T* element)
  {
    bool shift = false;
    for (uint16_t i = 0; i < size; i++)
    {
      if (shift)
      {
        data[i - 1] = data[i];
      }
      else if (data[i] == element)
      {
        shift = true;
      }
    }
    size--;
  }

  void clear()
  {
    size = 0;
  }

  uint16_t getSize() const
  {
    return size;
  }

  T** begin()
  {
    return data;
  }

  T** end()
  {
    return data + size;
  }

private:
  T * * data = NULL;
  uint16_t size = 0;
  uint16_t capacity;
  const bool ownData;
};


//-----------------------------------------------------------------------------
// Renderable
//-----------------------------------------------------------------------------

struct IRenderable
{
  virtual void draw(int16_t x, int16_t y) = 0;
};

class Renderable : public IRenderable
{
public:
  Renderable(int16_t originX = 0, int16_t originY = 0) :
    originX(originX),
    originY(originY)
  {
  }

  void setOrigin(int16_t x, int16_t y)
  {
    originX = x;
    originY = y;
  }

  int16_t originX;
  int16_t originY;
};

//-----------------------------------------------------------------------------
// Entity
//-----------------------------------------------------------------------------

#define _FLAG_ACTIVE 0x01
#define FLAG_COLLIDABLE 0x02
#define FLAG_VISIBLE 0x04
#define FLAG_1 0x08
#define FLAG_2 0x01
#define FLAG_3 0x02
#define FLAG_4 0x04
#define FLAG_5 0x08

struct IEntityPool;

class Entity : public IRenderable
{
public:
  virtual void update()
  {
  }

  virtual void draw(int16_t x, int16_t y)
  {
  }

  virtual void drawDebug(int16_t x, int16_t y);

  void remove();

  inline void setFlag(uint8_t flag, bool value)
  {
    if (value)
    {
      flags |= flag;
    }
    else
    {
      flags &= ~flag;
    }
  }

  inline bool getFlag(uint8_t flag)
  {
    return flags & flag;
  }

  uint8_t getType();

  int8_t hitboxX = 0;
  int8_t hitboxY = 0;
  uint8_t hitboxWidth = 0;
  uint8_t hitboxHeight = 0;

  void setHitbox(int8_t x, int8_t y, uint8_t width, uint8_t height)
  {
    hitboxX = x;
    hitboxY = y;
    hitboxWidth = width;
    hitboxHeight = height;
  }

  void setHitbox(uint8_t width, uint8_t height)
  {
    setHitbox(0, 0, width, height);
  }

  virtual bool collide(int16_t x, int16_t y, uint16_t w, uint16_t h) const;
  Entity* query(int16_t x, int16_t y, const uint8_t collideTypeIds[]) const;

  void moveBy(int16_t dx, int16_t dy, const uint8_t collideTypeIds[] = NULL);
  void moveTo(int16_t x, int16_t y, const uint8_t collideTypeIds[] = NULL);

  inline int16_t left()
  {
    return x + hitboxX;
  }

  inline int16_t right()
  {
    return left() + hitboxWidth;
  }

  inline int16_t top()
  {
    return y + hitboxY;
  }

  inline int16_t bottom()
  {
    return top() + hitboxHeight;
  }

  int16_t x = 0;
  int16_t y = 0;

  virtual void onInit()
  {
  }

protected:
  virtual bool onMoveCollideX(Entity& other)
  {
    return true;
  }

  virtual bool onMoveCollideY(Entity& other)
  {
    return true;
  }

private:
  uint8_t flags = 0;

public:
  void _init(int16_t x = 0, int16_t y = 0); // FIXME use friend
  IEntityPool * _pool = NULL; // FIXME use friend
};

//-----------------------------------------------------------------------------
// EntityPool
//-----------------------------------------------------------------------------

struct IEntityPool : public IRenderable
{
  virtual void remove(Entity* entity) = 0;

  virtual uint8_t getType() const = 0;
  virtual uint8_t getLayer() const = 0;

  virtual void update() = 0; 
  virtual void drawDebug(int16_t cameraX, int16_t cameraY) = 0;
  virtual Entity* query(int16_t x, int16_t y, uint16_t w, uint16_t h) = 0; // FIXME const  
};

struct IScene
{
  virtual void _addPool(IEntityPool* pool) = 0;
};

template<class T>
class EntityPool : public IEntityPool
{
public:
  EntityPool(IScene* scene, uint8_t type, uint16_t size, uint8_t layer = 0) :
    scene(scene),
    type(type),
    layer(layer),
    size(size),
    pool(new T[size])
  {
    scene->_addPool(this);
    for (uint16_t i = 0; i < size; i++)
    {
      pool[i]._pool = this;
    }
  }

  virtual ~EntityPool()
  {
    delete[] pool;
  }

  void _init()
  {
    for (uint16_t i = 0; i < size; i++)
    {
      pool[i].setFlag(_FLAG_ACTIVE, false);
    }
  } 

  T* spawn(int16_t x = 0, int16_t y = 0)
  {
    for (uint16_t i = 0; i < size; i++)
    {
      if (!pool[i].getFlag(_FLAG_ACTIVE))
      {
        pool[i]._init(x, y);
        return &pool[i];
      }
    }

    return NULL;
  }

  void remove(Entity* entity)
  {
    for (uint16_t i = 0; i < size; i++)
    {
      if (&pool[i] == entity)
      {
        pool[i].setFlag(_FLAG_ACTIVE, false);
        return;
      }
    }
  }

  T& get(uint16_t index = 0)
  {
    return pool[index];
  }

  uint8_t getType() const
  {
    return type;
  }

  uint8_t getLayer() const
  {
    return layer;
  }

  void update()
  {
    for (T* entity = begin(); entity < end(); entity++)
    {     
      if (entity->getFlag(_FLAG_ACTIVE))
      {
        entity->update();
      }
    }
  }

  void draw(int16_t x, int16_t y)
  {
    for (T* entity = begin(); entity < end(); entity++)
    {      
      if (entity->getFlag(_FLAG_ACTIVE) && entity->getFlag(FLAG_VISIBLE))
      {
        entity->draw(entity->x + x, entity->y + y);
      }
    }
  }

  void drawDebug(int16_t x, int16_t y)
  {
    for (T* entity = begin(); entity < end(); entity++)
    {    
      if (entity->getFlag(_FLAG_ACTIVE))
      {
        entity->drawDebug(entity->x + x, entity->y + y);
      }
    }
  }

  Entity* query(int16_t x, int16_t y, uint16_t w, uint16_t h)
  {
    for (T* entity = begin(); entity < end(); entity++)
    {      
      if (entity->getFlag(_FLAG_ACTIVE) && entity->getFlag(FLAG_COLLIDABLE) && entity->collide(x, y, w, h))
      {
        return entity;
      }
    }

    return NULL;
  }

private:
  IScene* const scene;
  const uint8_t type;
  const uint8_t layer;
  const uint16_t size;
  T* pool;

  T* begin()
  {
    return pool;
  }

  T* end()
  {
    return pool + size;
  }
};

//-----------------------------------------------------------------------------
// Scene
//-----------------------------------------------------------------------------

class Scene : public IScene
{
public:
  Scene();
  virtual ~Scene();

  void init();
  virtual void onInit()
  {
  }

  virtual void update();
  virtual void draw();
  virtual void drawDebug();

  int16_t cameraX = 0;
  int16_t cameraY = 0;

  void add(IRenderable& renderable, uint8_t layer = 0);
  // TODO remove(IRenderable* renderable);

  Entity* query(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t entityType); // FIXME const;
  Entity* query(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint8_t entityTypes[]); // FIXME const;

private:
  PtrVector<IEntityPool> pools;

public:
  void _addPool(IEntityPool* pool); // FIXME friend
};

//-----------------------------------------------------------------------------
// Sprite
//-----------------------------------------------------------------------------

#define LOOP 0
#define ONE_SHOT 1

class Sprite : public Renderable
{
public:
  Sprite(int16_t originX = 0, int16_t originY = 0) :
    Renderable(originX, originY)
  {
  }

  Sprite(const uint16_t *data, int16_t originX = 0, int16_t originY = 0) :
    Sprite(originX, originY)
  {
    init(data);
  }

  virtual void init(const uint16_t *data);

  virtual void draw(int16_t x, int16_t y);

  inline uint16_t getWidth() const
  {
    return width;
  }

  inline uint16_t getHeight() const
  {
    return height;
  }

  uint16_t frame = 0;

protected:
  const uint16_t* buffer = NULL;
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t transparentColor = 0;
};

//-----------------------------------------------------------------------------
// Anim
//-----------------------------------------------------------------------------

class Anim : public Renderable
{
public:
  Anim(int16_t originX = 0, int16_t originY = 0) :
    Renderable(originX, originY)
  {
  }

  Anim(const uint16_t *spriteData, const uint8_t* animData, int16_t originX = 0, int16_t originY = 0) :
    Anim(originX, originY)
  {
    init(spriteData, animData);
  }

  void init(const uint16_t *spriteData, const uint8_t* animData);
  void draw(int16_t x, int16_t y);

  void play(uint8_t anim);

  inline uint16_t getWidth() const
  {
    return sprite.getWidth();
  }

  inline uint16_t getHeight() const
  {
    return sprite.getHeight();
  }

  uint8_t frameOffset = 0;

private:
  const uint8_t* animData;
  const uint8_t* currentAnim;
  uint8_t currentFrameIndex;
  uint8_t counter;

  Sprite sprite;
};

//-----------------------------------------------------------------------------
// Tilemap
//-----------------------------------------------------------------------------

class Tilemap : public Renderable
{
public:
  Tilemap(int16_t originX = 0, int16_t originY = 0) :
    Renderable(originX, originY),
    data(NULL),
    width(0),
    height(0)
  {
  }

  Tilemap(const int16_t* mapData, const uint16_t* tilesetData, int16_t originX = 0, int16_t originY = 0) :
    Tilemap(originX, originY)
  {
    init(mapData, tilesetData);
  }

  void init(const int16_t* mapData, const uint16_t* tilesetData);

  inline int16_t getTile(int16_t x, int16_t y) const
  {
    return data[y * width + x];
  }

  void draw(int16_t x, int16_t y);

  inline uint8_t getWidth() const
  {
    return width;
  }

  inline uint8_t getHeight() const
  {
    return height;
  }

  inline uint16_t getTileWidth() const
  {
    return tileset.getWidth();
  }

  inline uint16_t getTileHeight() const
  {
    return tileset.getHeight();
  }

private:
  const int16_t* data;
  uint8_t width;
  uint8_t height;
  Sprite tileset;
};

//-----------------------------------------------------------------------------
// Core
//-----------------------------------------------------------------------------

namespace gbx
{
  void init(uint8_t frameRate = DEFAULT_FRAME_RATE);
  void update();

  // scene
  void setScene(Scene& scene);
  Scene& getScene();

  // display
  extern const int16_t width;
  extern const int16_t height;

  void clear(Color color = Color::black);

  void setPixel(int16_t x, int16_t y, Color c = Color::white);
  Color getPixel(int16_t x, int16_t y);

  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color c = Color::white);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, Color c = Color::white);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, Color c = Color::white);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, Color c = Color::white);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color c = Color::white);
  void drawCircle(int16_t x, int16_t y, int16_t r, Color c = Color::white);
  void fillCircle(int16_t x, int16_t y, int16_t r, Color c = Color::white);

  // input
  bool isDown(Gamebuino_Meta::Button button);
  bool wasPressed(Gamebuino_Meta::Button button);
  bool wasReleased(Gamebuino_Meta::Button button);
}

#endif
