#include "bnCardAction.h"
#include "bnAura.h"

class Character;

class AuraCardAction : public CardAction {
private:
  Aura::Type type;
public:
  AuraCardAction(Character& owner, Aura::Type type);
  ~AuraCardAction();
  void OnUpdate(double _elapsed);
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
};