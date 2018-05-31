#pragma once
#include "bnSpell.h"

class BasicSword : public Spell {
public:
  BasicSword(Field* _field, Team _team, int damage);
  virtual ~BasicSword(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Entity* _entity);
  virtual vector<Drawable*> GetMiscComponents();
  virtual int GetStateFromString(string _string);
  virtual void AddAnimation(int _state, FrameAnimation _animation, float _duration);

private:
  int damage;
};
