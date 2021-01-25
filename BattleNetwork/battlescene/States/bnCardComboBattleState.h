#pragma once

#include <Swoosh/Timer.h>
#include <memory>

#include "../bnBattleSceneState.h"
#include "../../bnSelectedCardsUI.h"
#include "../../bnPA.h"

/*
    This state spells out the combo steps
 */
struct CardComboBattleState final : public BattleSceneState {

  bool paChecked{ false };
  bool isPAComplete{ false };
  bool advanceSoundPlay{ false };
  int hasPA{ -1 };
  int paStepIndex{ 0 };
  int* cardCountPtr{ nullptr };
  double elapsed{ 0 };
  double increment{ 0 }; /*!< Used to cycle through colors for the PA label */
  double listStepCooldown{ 0.25 };
  double listStepCounter{ 0 };
  double PAStartLength{ 0.25 }; /*!< Total time to animate PA in seconds */
  PA& programAdvance; /*!< PA object loads PA database and returns matching PA card from input */
  PA::Steps paSteps; /*!< Matching steps in a PA */
  swoosh::Timer PAStartTimer; /*!< Time to scale the PA graphic */
  sf::Sprite programAdvanceSprite;
  Font font;
  SelectedCardsUI& ui;
  Battle::Card*** cardsListPtr{ nullptr };

  CardComboBattleState(SelectedCardsUI& ui, PA& programAdvance);
  void ShareCardList(Battle::Card*** cardsPtr, int* listLengthPtr);
  void onStart(const BattleSceneState* last) override;
  void onEnd(const BattleSceneState* next) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsDone();
};