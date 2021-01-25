#pragma once
#include "bnEntity.h"
using sf::Texture;

/**
 * @brief Artifacts do not attack and they are not living. They are tile-rooted animations purely for visual effect.
 */
class Artifact : public Entity {
public:
  Artifact();
  virtual ~Artifact();

  virtual void OnUpdate(double _elapsed) = 0;
  virtual void OnDelete() = 0;
  virtual void Update(double _elapsed) final;
  virtual void AdoptTile(Battle::Tile* tile) final;
};
