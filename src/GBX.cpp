#include "GBX.h"

//-----------------------------------------------------------------------------
// Core
//-----------------------------------------------------------------------------

namespace // unamed
{
  Scene * scene;
  PtrVector<PtrVector<IRenderable>> layers(RENDERABLES_BY_LAYER_INITIAL_CAPACITY, true);
  uint16_t entityCount; // FIXME
  uint8_t debugLevel = 0;
} // unamed

const int16_t gbx::width = gb.display.width();
const int16_t gbx::height = gb.display.height();

void gbx::init(uint8_t frameRate)
{
  gb.begin();
  gb.setFrameRate(frameRate);
}

void gbx::update()
{
  while (!gb.update());

  if (wasPressed(BUTTON_MENU))
  {
    debugLevel = (debugLevel + 1) % 3;
  }

  if (scene != NULL)
  {
    scene->update();
    scene->draw();
  }

  if (debugLevel > 0)
  {
    if (debugLevel > 1 && scene != NULL)
    {
      scene->drawDebug();
    }

#ifndef GBMSDL // TODO support print in GBMSDL
    gb.display.setCursor(0, 0);
    gb.display.setColor(Color::white);
    gb.display.print("cpu=");
    gb.display.println(gb.getCpuLoad());
    gb.display.print("ram=");
    gb.display.println(gb.getFreeRam());
    gb.display.print("cnt=");
    gb.display.println(entityCount);
#endif
  }
}

void gbx::setScene(Scene& scene)
{
  entityCount = 0;

  ::scene = &scene;
  scene.init();
}

Scene& gbx::getScene()
{
  return *::scene;
}

void gbx::clear(Color color)
{
  gb.display.clear(color);
}

void gbx::setPixel(int16_t x, int16_t y, Color c)
{
  gb.display.drawPixel(x, y, c);
}

Color gbx::getPixel(int16_t x, int16_t y)
{
  return gb.display.getPixelColor(x, y);
}

void gbx::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color c)
{
  gb.display.setColor(c);
  gb.display.drawLine(x0, y0, x1, y1);
}

void gbx::drawFastVLine(int16_t x, int16_t y, int16_t h, Color c)
{
  gb.display.setColor(c);
  gb.display.drawFastVLine(x, y, h);
}

void gbx::drawFastHLine(int16_t x, int16_t y, int16_t w, Color c)
{
  gb.display.setColor(c);
  gb.display.drawFastHLine(x, y, w);
}

void gbx::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, Color c)
{
  gb.display.setColor(c);
  gb.display.drawRect(x, y, w, h);
}

void gbx::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color c)
{
  gb.display.setColor(c);
  gb.display.fillRect(x, y, w, h);
}

void gbx::drawCircle(int16_t x, int16_t y, int16_t r, Color c)
{
  gb.display.setColor(c);
  gb.display.drawCircle(x, y, r);
}

void gbx::fillCircle(int16_t x, int16_t y, int16_t r, Color c)
{
  gb.display.setColor(c);
  gb.display.fillCircle(x, y, r);
}

bool gbx::isDown(Gamebuino_Meta::Button button)
{
  return gb.buttons.repeat(button, 0);
}

bool gbx::wasPressed(Gamebuino_Meta::Button button)
{
  return gb.buttons.pressed(button);
}

bool gbx::wasReleased(Gamebuino_Meta::Button button)
{
  return gb.buttons.released(button);
}

//-----------------------------------------------------------------------------
// Entity
//-----------------------------------------------------------------------------

void Entity::_init(int16_t x, int16_t y)
{
  this->x = x;
  this->y = y;
  flags = _FLAG_ACTIVE | FLAG_COLLIDABLE | FLAG_VISIBLE;
  onInit();
}

void Entity::remove()
{
  _pool->remove(this);
}

uint8_t Entity::getType()
{
  return _pool->getType();
}

void Entity::drawDebug(int16_t x, int16_t y)
{
  gbx::drawRect(x + hitboxX, y + hitboxY, hitboxWidth, hitboxHeight, Color::lightblue);
  gbx::setPixel(x, y, Color::red);
}

bool Entity::collide(int16_t x, int16_t y, uint16_t w, uint16_t h) const
{
  return gb.collideRectRect(Entity::x + hitboxX, Entity::y + hitboxY, hitboxWidth, hitboxHeight, x, y, w, h);
}

Entity* Entity::query(int16_t x, int16_t y, const uint8_t collideTypeIds[]) const
{
  return gbx::getScene().query(x + hitboxX, y + hitboxY, hitboxWidth, hitboxHeight, collideTypeIds);
}

void Entity::moveBy(int16_t dx, int16_t dy, const uint8_t collideTypeIds[])
{
  if (collideTypeIds == NULL)
  {
    x += dx;
    y += dy;
    return;
  }

  if (dx != 0)
  {
    int16_t sign = dx < 0 ? -1 : 1;
    int16_t dist = dx * sign;
    while (dist > 0)
    {
      Entity* other = query(x + sign, y, collideTypeIds);
      if (other != NULL && onMoveCollideX(*other))
      {
        break;
      }
      dist--;
      x += sign;
    }
  }

  if (dy != 0)
  {
    int16_t sign = dy < 0 ? -1 : 1;
    int16_t dist = dy * sign;
    while (dist > 0)
    {
      Entity* other = query(x, y + sign, collideTypeIds);
      if (other != NULL && onMoveCollideY(*other))
      {
        break;
      }
      dist--;
      y += sign;
    }
  }
}

void Entity::moveTo(int16_t x, int16_t y, const uint8_t collideTypeIds[])
{
  moveBy(x - Entity::x, y - Entity::y, collideTypeIds);
}

//-----------------------------------------------------------------------------
// Scene
//-----------------------------------------------------------------------------

Scene::Scene() :
  pools(TYPES_INITIAL_CAPACITY)
{
}

Scene::~Scene()
{
}

void Scene::init()
{
  for (auto renderables = layers.begin(); renderables != layers.end(); renderables++)
  {
    (*renderables)->clear();
  }

  for (IEntityPool** pool = pools.begin(); pool < pools.end(); pool++)
  {
    if (*pool != NULL)
    {
      add(**pool, (*pool)->getLayer());
    }
  }

  onInit();
}

void Scene::_addPool(IEntityPool * pool)
{
  pools[pool->getType()] = pool;
}

void Scene::add(IRenderable& renderable, uint8_t layer)
{
  PtrVector<IRenderable>* renderables = layers[layer];
  if (renderables == NULL)
  {
    renderables = new PtrVector<IRenderable>(RENDERABLES_BY_LAYER_INITIAL_CAPACITY);
    layers[layer] = renderables;
  }
  renderables->add(&renderable);
}

void Scene::update()
{
  for (IEntityPool** pool = pools.begin(); pool < pools.end(); pool++)
  {
    if (*pool != NULL)
    {
      (*pool)->update();
    }
  }
}

void Scene::draw()
{
  gbx::clear();

  for (PtrVector<IRenderable>** renderables = layers.begin(); renderables < layers.end(); renderables++)
  {
    if (*renderables != NULL)
    {
      for (IRenderable** renderable = (*renderables)->begin(); renderable < (*renderables)->end(); renderable++)
      {
        (*renderable)->draw(-cameraX, -cameraY);
      }
    }
  }
}

void Scene::drawDebug()
{
  for (IEntityPool** pool = pools.begin(); pool < pools.end(); pool++)
  {
    if (*pool != NULL)
    {
      (*pool)->drawDebug(-cameraX, -cameraY);
    }
  }
}

Entity* Scene::query(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t entityType)
{
  if (entityType >= pools.getSize())
  {
    return NULL;
  }

  IEntityPool* pool = pools[entityType];
  if (pool != NULL)
  {
    Entity* entity = pool->query(x, y, w, h);
    if (entity != NULL)
    {
      return entity;
    }
  }

  return NULL;
}

Entity* Scene::query(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint8_t entityTypes[])
{
  for (uint8_t i = 1; i <= entityTypes[0]; i++)
  {
    Entity* entity = query(x, y, w, h, entityTypes[i]);
    if (entity != NULL)
    {
      return entity;
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------
// Sprite
//-----------------------------------------------------------------------------

// TODO add support for horiz/vert flip

void Sprite::init(const uint16_t *data)
{
  width = pgm_read_word(data++);
  height = pgm_read_word(data++);
  transparentColor = pgm_read_word(data++);
  buffer = data;
}

void Sprite::draw(int16_t x, int16_t y)
{
  x += this->originX;
  y += this->originY;

  if ((x > gbx::width) || ((x + width) < 0) || (y > gbx::height) || ((y + height) < 0))
  {
    // out of screen!
    return;
  }

  // horizontal cropping (same as in GameBuino-Meta.h)
  int16_t xOffset = 0;
  int16_t renderWidth = width;
  if (x < 0)
  {
    xOffset = -x;
    renderWidth = width + x;
    if (renderWidth > gbx::width)
    {
      renderWidth = gbx::width;
    }
  }
  else if ((x + width) > gbx::width)
  {
    renderWidth = gbx::width - x;
  }

  // vertical cropping (same as in GameBuino-Meta.h)
  int16_t yOffset = 0;
  int16_t renderHeight = height;
  if (y < 0)
  {
    yOffset = -y;
    renderHeight = height + y;
    if (renderHeight > gbx::height)
    {
      renderHeight = gbx::height;
    }
  }
  else if ((y + height) > gbx::height)
  {
    renderHeight = gbx::height - y;
  }

  // calculate source and dest pointers initial address
  const uint16_t* sourcePtr = buffer + (yOffset * width) + xOffset;
  if (frame > 0)
  {
    sourcePtr += frame * width * height;
  }
  uint16_t* destPtr = gb.display._buffer + (y + yOffset) * gb.display.width() + x + xOffset;

  if (!transparentColor)
  {
    // no transparent color, use memcpy
    for (uint8_t iy = 0; iy < renderHeight; iy++)
    {
      memcpy(destPtr, sourcePtr, renderWidth * 2);
      sourcePtr += width;
      destPtr += gb.display.width();
    }
  }
  else
  {
    // transparent color, copy pixel by pixel
    for (uint8_t iy = 0; iy < renderHeight; iy++)
    {
      for (uint8_t ix = 0; ix < renderWidth; ix++)
      {
        if (*sourcePtr != transparentColor)
        {
          *destPtr = *sourcePtr;
        }
        sourcePtr++;
        destPtr++;
      }
      sourcePtr += width - renderWidth;
      destPtr += gb.display.width() - renderWidth;
    }
  }
}

//-----------------------------------------------------------------------------
// Anim
//-----------------------------------------------------------------------------

void Anim::init(const uint16_t *spriteData, const uint8_t* animData)
{
  sprite.init(spriteData);
  this->animData = animData;
  currentAnim = NULL;
}

void Anim::draw(int16_t x, int16_t y)
{
  if (currentAnim == NULL)
  {
    return;
  }

  uint8_t interval = currentAnim[2];
  if (interval > 0)
  {
    if (++counter == interval)
    {
      if (++currentFrameIndex == currentAnim[0])
      {
        if (currentAnim[1] == LOOP)
        {
          currentFrameIndex = 0;
        }
        else // currentAnim[1] == ONE_SHOT
        {
          currentAnim = NULL;
          return;
        }

      }
      counter = 0;
    }
  }

  sprite.frame = currentAnim[3 + currentFrameIndex] + frameOffset;
  sprite.draw(x + this->originX, y + this->originY);
}

void Anim::play(uint8_t anim)
{
  currentAnim = animData;
  for (uint8_t i = 0; i < anim; i++)
  {
    currentAnim += 3 + currentAnim[0];
  }
  currentFrameIndex = 0;
  counter = 0;
}

//-----------------------------------------------------------------------------
// Tilemap
//-----------------------------------------------------------------------------

void Tilemap::init(const int16_t* mapData, const uint16_t* tilesetData)
{
  Tilemap::width = (uint8_t) *(mapData++);
  Tilemap::height = (uint8_t) *(mapData++);
  data = mapData;
  tileset.init(tilesetData);
}

void Tilemap::draw(int16_t x, int16_t y)
{
  x += this->originX;
  y += this->originY;

  int16_t startX = -x / getTileWidth();
  int16_t startY = -y / getTileHeight();
  int16_t endX = startX + gbx::width / getTileWidth() + 1;
  int16_t endY = startY + gbx::height / getTileHeight() + 1;
  if (startX < 0) startX = 0;
  if (startY < 0) startY = 0;
  if (endX > width) endX = width;
  if (endY > height) endY = height;
  for (int16_t ix = startX; ix < endX; ix++)
  {
    for (int16_t iy = startY; iy < endY; iy++)
    {
      int16_t tid = getTile(ix, iy);
      if (tid >= 0)
      {
        tileset.frame = tid;
        tileset.draw(ix * getTileWidth() + x, iy * getTileHeight() + y);
      }
    }
  }
}
